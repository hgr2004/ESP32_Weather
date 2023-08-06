/* *****************************************************************
 *
 * SmallDesktopDisplay
 *    小型桌面显示器
 *
 * 原  作  者：Misaka
 * 修      改：微车游
 * 讨  论  群：811058758、887171863
 * 创 建 日 期：2021.07.19
 * 最后更改日期：2021.09.14
 * 更 改 说 明：V1.1添加串口调试，波特率115200\8\n\1；增加版本号显示。
 *            V1.2亮度和城市代码保存到EEPROM，断电可保存
 *            V1.3.1 更改smartconfig改为WEB配网模式，同时在配网的同时增加亮度、屏幕方向设置。
 *            V1.3.2 增加wifi休眠模式，仅在需要连接的情况下开启wifi，其他时间关闭wifi。增加wifi保存至eeprom（目前仅保存一组ssid和密码）
 *            V1.3.3  修改WiFi保存后无法删除的问题。目前更改为使用串口控制，输入 0x05 重置WiFi数据并重启。
 *                    增加web配网以及串口设置天气更新时间的功能。
 *            V1.3.4  修改web配网页面设置，将wifi设置页面以及其余设置选项放入同一页面中。
 *                    增加web页面设置是否使用DHT传感器。（使能DHT后才可使用）
 *
 * 引 脚 分 配： SCK  GPIO14
 *              MOSI  GPIO15
 *              RES   GPIO33
 *              DC    GPIO27
 *              LCDBL GPIO22
 *              CS    GPIO5
 *
 *             增加DHT11温湿度传感器，传感器接口为 GPIO 13
 *
 *    感谢群友 @你别失望  提醒发现WiFi保存后无法重置的问题，目前已解决。详情查看更改说明！

 *  烧录方式：
 * 使用USB转TTL烧录器
 * 板子上G接烧录器GND
 * 板子上3接烧录器3.3v
 * 板子上T接烧录器RXD
 * 板子上R接烧录器TXD
 * 烧录时按住boot键不放再按一下en键即可进入下载模式

 * *****************************************************************/

/* *****************************************************************
 *  库文件、头文件
 * *****************************************************************/
#include <Arduino.h>
#include <string.h>
#include "ArduinoJson.h"
#include <TimeLib.h>

// Gzip相关定义
#ifndef ESP32
#error "gzStreamExpander is only available on ESP32 architecture"
#endif
#define DEST_FS_USES_LITTLEFS

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <EEPROM.h>
#include "qr.h"
#include "number.h"
#include "weathernum.h"
#include <NTPClient.h>
#include <ESP32Time.h>
#include <ESP32-targz.h>
#include "esp32-hal-cpu.h"
#include <DigitalRainAnimation.hpp>

#include "WeatherWarn.h"
#include "HttpsGetUtils.h"

// Font files are stored in Flash FS
#include <FS.h>
#include <LittleFS.h>
#define FlashFS LittleFS

#define Version "SDD V1.8.8"
#define MaxScroll 50 // 定义最大滚动显示条数

/* *****************************************************************
 *  请前往相关的网站申请key
 * *****************************************************************/
// 和风天气的key 申请地址： https://dev.qweather.com/docs/start/
String HeUserKey = ""; 
WeatherWarn weatherWarn;

//电点工作室/mxnzp.com  申请地址：https://www.mxnzp.com/   一个个人维护的站点
String mx_id = "";
String mx_secret = "";

/* *****************************************************************
 *  配置使能位
 * *****************************************************************/
// WEB配网使能标志位----WEB配网打开后会默认关闭smartconfig功能
#define WM_EN 1
// Web服务器使能标志位----打开后将无法使用wifi休眠功能。
#define WebSever_EN 1
// 设定DHT11温湿度传感器使能标志
#define DHT_EN 1
// 设置太空人图片是否使用
#define imgAst_EN 1

#if WM_EN
#include <WiFiManager.h>
// WiFiManager 参数
WiFiManager wm; // global wm instance
#endif

#if WebSever_EN
// #include "ESPAsyncWebServer.h"
// #include <DNSServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WebServer.h>

// 设置ESP32服务器运行于80端口
// AsyncWebServer server(80);
WebServer server(80); // 建立esp8266网站服务器对象
#endif

// 设定DHT11温湿度传感器引脚
#if DHT_EN
#include "DHT.h"
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#endif

/* *****************************************************************
 *  字库、图片库
 * *****************************************************************/
#include "font/ZdyLwFont_20.h"
#include "font/ZdXiao.h"
#include "img/misaka.h"
#include "img/temperature.h"
#include "img/humidity.h"

#if imgAst_EN
#include "img/pangzi/i0.h"
#include "img/pangzi/i1.h"
#include "img/pangzi/i2.h"
#include "img/pangzi/i3.h"
#include "img/pangzi/i4.h"
#include "img/pangzi/i5.h"
#include "img/pangzi/i6.h"
#include "img/pangzi/i7.h"
#include "img/pangzi/i8.h"
#include "img/pangzi/i9.h"

int Anim = 0;      // 太空人图标显示指针记录
unsigned long AprevTime = 0; // 太空人更新时间记录
#endif

/* *****************************************************************
 *  参数设置
 * *****************************************************************/

struct config_type
{
  char stassid[32]; // 定义配网得到的WIFI名长度(最大32字节)
  char stapsw[64];  // 定义配网得到的WIFI密码长度(最大64字节)
};

//---------------修改此处""内的信息--------------------
// 如开启WEB配网则可不用设置这里的参数，前一个为wifi ssid，后一个为密码
config_type wificonf = {{""}, {""}};

//----------------------------------------------------
// LCD屏幕相关设置
TFT_eSPI tft = TFT_eSPI(); // 引脚请自行配置tft_espi库中的 User_Setup.h文件
TFT_eSprite clk = TFT_eSprite(&tft);

// 黑客帝国数字雨效果
DigitalRainAnimation<TFT_eSPI> matrix_effect = DigitalRainAnimation<TFT_eSPI>();

#define LCD_BL_PIN 22 // LCD背光引脚
#define MINLIGHT 88   // 背光最小亮度-定时器夜晚调到最小
const uint16_t bgColor = 0x0000;

// 屏幕亮度调节
int pwm_channel0 = 0;
int pwm_freq = 5000;
// int pwm_value = 10;    //0-255

// 其余状态标志位
int LCD_Rotation = 0;        // LCD屏幕方向
int LCD_BL_PWM = 250;        // 屏幕亮度0-255，默认250
uint8_t Wifi_en = 1;         // wifi状态标志位  1：打开    0：关闭
uint8_t UpdateWeater_en = 0; // 更新时间标志位
bool isNewWeather = false;   // 天气更新后
bool isNewWarn = false;      // 预警更新标志
int prevTime = 1;            // 滚动显示更新标志位
int DHT_img_flag = 0;        // DHT传感器使用标志位
bool UpdateScreen = 0;       // 全部重画屏幕

time_t prevDisplay = 0;       // 显示时间显示记录
unsigned long weaterTime = 0; // 天气更新时间记录

String scrollText[7] = {""}; // 天气情况滚动显示数组

/*** Component objects ***/
Number dig;
WeatherNum wrat;

// 增加和风天气的预警功能
uint32_t targetTime = 0;
String cityCode = "101281006"; // 天气城市代码 湛江： 101281001 长沙: 101250101 株洲: 101250301 衡阳: 101250401 赤坎： 101281006  霞山 101281009
int tempnum = 0;               // 温度百分比
int huminum = 0;               // 湿度百分比
int tempcol = 0xffff;          // 温度显示颜色
int humicol = 0xffff;          // 湿度显示颜色
int pm25V = 0;                     // PM2.5
int Iconsname;                 // 天气图标名称
String cityname = "";               // 城市名称
// 天气更新时间  默认20分钟
int updateweater_time = 20;

// EEPROM参数存储地址位
int BL_addr = 1;    // 被写入数据的EEPROM地址编号  1亮度
int Ro_addr = 2;    // 被写入数据的EEPROM地址编号  2 旋转方向
int DHT_addr = 3;   // 被写入数据的EEPROM地址编号  3 DHT使能标志位
int UpWeT_addr = 4; // 更新时间记录
int CC_addr = 10;   // 被写入数据的EEPROM地址编号  10城市
int wifi_addr = 30; // 被写入数据的EEPROM地址编号  20wifi-ssid-psw

// wifi连接UDP设置参数
WiFiUDP Udp;
WiFiClient wificlient;
unsigned int localPort = 8321;
float duty = 0;

// NTP服务器参数
const int timeZone = 8; // 东八区

//WiFiUDP ntpUDP;
// IPAddress NtpIP = IPAddress(210,72,145,44);  //国家授时中心
NTPClient timeClient(Udp, "ntp.ntsc.ac.cn", 60 * 60 * timeZone, 12 * 60 *  60 * 1000);
ESP32Time rtc; // 用来管理系统时间

hw_timer_t *timer = NULL; // 声明一个定时器用来取NTP
unsigned int updateTime = 0;

// 进度条
byte loadNum = 6;

// 设计一个结构体，实现不同意思字显示不同颜色
#define cRED 0
#define cGREEN 1
#define cWHITE 2

typedef struct
{
  int color;
  String title;
} Display;

#define LENGTH(array) (sizeof(array) / sizeof(array[0]))

Display *scrollNongLi = NULL;
Display *scrolHEAD = NULL;
Display *scrollYI = NULL;
Display *scrollJI = NULL;
int TotalDis = 0;
int TotalHEAD = 0;
int TotalYI = 0;
int TotalJI = 0;
int CurrentDisDate = 0;

// 绘制预警画面有关变量定义
#define DEG2RAD 0.0174532925
#define LOOP_DELAY 1 // Loop delay to slow things down

byte inc = 0;
unsigned int col = 0;

byte red = 31;  // Red is the top 5 bits of a 16 bit colour value
byte green = 0; // Green is the middle 6 bits
byte blue = 0;  // Blue is the bottom 5 bits
byte state = 0;

/* *********************************************************/
/*  ***************函数定义**********************************/
/* *********************************************************/
void getCityCode();
void getCityWeater();
void saveParamCallback();
void scrollBanner();
void scrollDate();
void imgAnim();
void weaterData();
String monthDay();
String week();
void getNongli();
/* *********************************************************/
/*  ********************************************************/
/* *********************************************************/
// 函数声明
time_t getNtpTime();
void digitalClockDisplay(int reflash_en);
void printDigits(int digits);
String num2str(int digits);
void sendNTPpacket(IPAddress &address);
void LCD_reflash(bool en);
void savewificonfig();
void readwificonfig();
void deletewificonfig();
int StrSplit(String str, String fen, String *result);
void IRAM_ATTR onTimer();
void ledcAnalogWrite(uint8_t channel, uint32_t value);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void Web_win();
void Webconfig();
void loading(byte delayTime); // 绘制进度条
void IndoorTem();
void Serial_set();
void sleepTimeLoop(uint8_t Maxlight, uint8_t Minlight);
void taskA(void *ptParam);
void taskB(void *ptParam);
void getDHT11();
void myTarProgressCallback(uint8_t progress);
void my_strcat_arrcy(Display *arr, int lena, Display *brr, int lenb, Display *crr, int lenc, Display *str);
String HTTPS_request(String host, String url, String parameter);
void getWarning();
void DispWarn(int en);
void Wait_win(String showStr);

void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour);
unsigned int brightness(unsigned int colour, int brightness);
unsigned int rainbow(byte value);
void SaveConfigCallback();

#if WebSever_EN
void Web_Sever_Init();
void Web_Sever();
void Web_sever_Win();
void saveCityCodetoEEP(int *citycode);
void readCityCodefromEEP(int *citycode);
#endif
/* *********************************************************/

void setup()
{
  Serial.begin(115200);

  setCpuFrequencyMhz(240);
  Serial.print("CPU频率是： ");
  Serial.println(getCpuFrequencyMhz());
 
  tft.begin();          /* TFT init */
  tft.invertDisplay(1); // 反转所有显示颜色：1反转，0正常
  tft.setRotation(LCD_Rotation);
  tft.initDMA();
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_BLACK, bgColor);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  if (!LittleFS.begin())
  {
    Serial.println("Flash FS initialisation failed!");
  }
  else
  {
    Serial.println("Flash FS available!");
  }
  // 显示开机LOGO
  TJpgDec.drawFsJpg(0, 0, "/logo.jpg", LittleFS);
  delay(3000); // 花一些时间打开串行监视器

  EEPROM.begin(1024);

  // 屏幕亮度控制初始化
  //  initialize digital pin LED_BUILTIN as an output.
  ledcSetup(pwm_channel0, pwm_freq, 13);
  ledcAttachPin(LCD_BL_PIN, pwm_channel0);

  // 设置一个定时器处理定时任务 1秒
  timer = timerBegin(0, 80, true);             // 初始化定时器指针
  timerAttachInterrupt(timer, &onTimer, true); // 绑定定时器
  timerAlarmWrite(timer, 1000000, true);       // 配置报警计数器保护值（就是设置时间）单位uS
  timerAlarmEnable(timer);                     // 启用定时器

#if DHT_EN
  dht.begin();
  // 从eeprom读取DHT传感器使能标志
  DHT_img_flag = EEPROM.read(DHT_addr);
#endif
  // 从eeprom读取天气更新时间间隔设置
  if (EEPROM.read(UpWeT_addr) > 0 && EEPROM.read(UpWeT_addr) < 60)
    updateweater_time = EEPROM.read(UpWeT_addr);
  // 从eeprom读取背光亮度设置
  if (EEPROM.read(BL_addr) > 0 && EEPROM.read(BL_addr) < 255)
    LCD_BL_PWM = EEPROM.read(BL_addr);
  // 从eeprom读取屏幕方向设置
  if (EEPROM.read(Ro_addr) >= 0 && EEPROM.read(Ro_addr) <= 3)
    LCD_Rotation = EEPROM.read(Ro_addr);
  // 设置背光
  ledcAnalogWrite(pwm_channel0, LCD_BL_PWM);

  // 黑客帝国效果
  matrix_effect.init(&tft);
  matrix_effect.setup(
      10 /* Line Min */,
      30, /* Line Max */
      5,  /* Speed Min */
      25, /* Speed Max */
      50 /* Screen Update Interval */);

  targetTime = millis() + 1000;
  readwificonfig(); // 读取存储的wifi信息
  Serial.print("正在连接WIFI ");
  Serial.println(wificonf.stassid);

  WiFi.begin(wificonf.stassid, wificonf.stapsw);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(true);

  tft.fillScreen(bgColor);
  while (WiFi.status() != WL_CONNECTED)
  {
    loading(30);

    if (loadNum >= 197)
    {
// 使能web配网后自动将smartconfig配网失效
#if WM_EN
      Web_win();
      Webconfig();
      tft.fillScreen(bgColor);
#endif

#if !WM_EN
      SmartConfig();
#endif
      break;
    }
  }

  while (loadNum < 194) // 让动画走完
  {
    loading(1);
  }

  Wait_win("WIFI已连接......"); // 显示连接成功后界面

  Serial.print("本地IP： ");
  Serial.println(WiFi.localIP());
  Serial.println("启动UDP");
  Udp.begin(localPort);
  Serial.println("等待同步...");

  timeClient.begin(); 
  //rtc.setTime(getNtpTime());
  
  Wait_win("等待时间服务器..."); // 显示连接成功后界面

  loadNum = 20;
  while(rtc.getYear() == 1970)
  {
    
    if (millis() - AprevTime > 500) // x ms切换一次
    {
      AprevTime = millis();
      Wait_win("正在同步时间...");        
      //rtc.setTime(getNtpTime());    
      getNtpTime();
      if(timeClient.isTimeSet()) 
        Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S")); // (String) returns time with specified format
      loadNum += 19;    
    }
    if (loadNum >= 170)
    {
      Serial.println("获取NTP时间失败，手动设置一个时间：2023-6-18-1-1-1"); // 如果同步失败，手动设置一个系统时间。
      rtc.setTime(01, 01, 01, 18, 6, 2023);   // 17th Jan 2021 15:24:30       
      loadNum = 194;                       
      break;
    }
  }
 
  while (loadNum < 170) // 让动画走完
  {
    Wait_win("时间同步成功...");
    loadNum += 39; 
  }
  loadNum = 194;

  // 每 60 * 60 秒同步时间一次
  setSyncProvider(getNtpTime);
  setSyncInterval(60 * 60); // 每60分钟同步一次时间  

  Wait_win("正在城市信息..."); // 显示连接成功后界面
  // 获取城市代码
  int CityCODE = 0;
  for (int cnum = 5; cnum > 0; cnum--)
  {
    CityCODE = CityCODE * 100;
    CityCODE += EEPROM.read(CC_addr + cnum - 1);
    delay(5);
  }
  if (CityCODE >= 101000000 && CityCODE <= 102000000)
    cityCode = CityCODE;
  else
    getCityCode(); // 获取城市代码
  Wait_win("正在获取农历信息......"); // 显示连接成功后界面
  getNongli();                        // 农历信息
  Wait_win("正在获取天气情况......"); // 显示连接成功后界面
  getCityWeater();                    // 取天气情况
  Wait_win("正在获取预警信息......"); // 显示连接成功后界面
  // 使用城市ID取当前预警
  weatherWarn.config(HeUserKey, cityCode); // 配置请求信息  101230201厦门 101230201 厦门  101281006 湛江 101281009 霞山
  getWarning();                            // 取当前预警
  Wait_win("等待启动WEB服务...");       // 显示连接成功后界面

#if WebSever_EN
  // 开启web服务器初始化
  Web_Sever_Init();
  Web_sever_Win();
  delay(6000);
#endif

#if DHT_EN
  if (DHT_img_flag != 0){
    getDHT11();  
    IndoorTem();    
  }
#endif  

  // 任务A用来采集DHT11的温度湿度 tskNO_AFFINITY 表示不指定核心
  xTaskCreatePinnedToCore(taskA, "Task A", 1600 * 1, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskB, "Task B", 1024 * 2, NULL, 1, NULL, 1);
  tft.fillScreen(TFT_BLACK); // 清屏
}

void loop()
{
  LCD_reflash(UpdateScreen);
#if WebSever_EN
  Web_Sever();
#endif
  Serial_set();
  DispWarn(isNewWarn);
}

  // Serial.println("check 1:");
  // Serial.println(ESP.getFreeHeap());

/* ***************************************************************************
 *  以下各类函数
 *
 * **************************************************************************/
// 任务A用来采集DHT11的温度湿度
void taskA(void *ptParam)
{

  TickType_t xLastWakeTime;
  const TickType_t xDelayms = pdMS_TO_TICKS(10000); // 10秒钟采一次
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, xDelayms);
    getDHT11();
  }
}

// 任务B用来*********
void taskB(void *ptParam)
{
  TickType_t xLastWakeTime;
  const TickType_t xDelayms = pdMS_TO_TICKS(1000); // 1000ms
  xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, xDelayms);
    sleepTimeLoop(LCD_BL_PWM, MINLIGHT); // 定时开关显示屏背光 参数是打开后最大亮度
    // printf("TaskB剩余栈%d\r\n", uxTaskGetStackHighWaterMark(NULL)); // uxTaskGetStackHighWaterMark以word为单位
    // printf("xPortGetFreeHeapSize = %d\r\n", xPortGetFreeHeapSize());
    // printf("xPortGetMinimumEverFreeHeapSize = %d\r\n", xPortGetMinimumEverFreeHeapSize());
  }
}

void IRAM_ATTR onTimer()
{               // 定时器中断函数
  updateTime++; // 加1秒
}

/* *****************************************************************
 *  函数
 * *****************************************************************/
// wifi ssid，psw保存到eeprom
void savewificonfig()
{
  // 开始写入
  uint8_t *p = (uint8_t *)(&wificonf);
  for (int i = 0; i < sizeof(wificonf); i++)
  {
    EEPROM.write(i + wifi_addr, *(p + i)); // 在闪存内模拟写入
  }
  delay(10);
  EEPROM.commit(); // 执行写入ROM
  delay(10);
}
// 删除原有eeprom中的信息
void deletewificonfig()
{
  config_type deletewifi = {{""}, {""}};
  uint8_t *p = (uint8_t *)(&deletewifi);
  for (int i = 0; i < sizeof(deletewifi); i++)
  {
    EEPROM.write(i + wifi_addr, *(p + i)); // 在闪存内模拟写入
  }
  delay(10);
  EEPROM.commit(); // 执行写入ROM
  delay(10);
}

// 从eeprom读取WiFi信息ssid，psw
void readwificonfig()
{
  uint8_t *p = (uint8_t *)(&wificonf);
  for (int i = 0; i < sizeof(wificonf); i++)
  {
    *(p + i) = EEPROM.read(i + wifi_addr);
  }
  Serial.printf("Read WiFi Config.....\r\n");
  Serial.printf("SSID:%s\r\n", wificonf.stassid);
  Serial.print("************");
  Serial.printf("Connecting.....\r\n");
}
// TFT屏幕输出函数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  if (y >= tft.height())
    return 0;
  tft.pushImage(x, y, w, h, bitmap);
  // Return 1 to decode next block
  return 1;
}

// 进度条函数
void loading(byte delayTime) // 绘制进度条
{
  clk.setColorDepth(8);

  clk.createSprite(200, 100); // 创建窗口
  clk.fillSprite(0x0000);     // 填充率

  clk.drawRoundRect(0, 0, 200, 16, 8, 0xFFFF);     // 空心圆角矩形
  clk.fillRoundRect(3, 3, loadNum, 10, 5, TFT_GREEN); // 实心圆角矩形
  clk.setTextDatum(CC_DATUM);                      // 设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("Connecting to WiFi......", 100, 40, 2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawRightString(Version, 180, 60, 2);
  clk.pushSprite(20, 120); // 窗口位置
  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}

// 湿度图标显示函数
void humidityWin()
{
  clk.setColorDepth(8);
  huminum = huminum * (44 - 1) / 100;
  clk.createSprite(44, 6);                         // 创建窗口
  clk.fillSprite(0x0000);                          // 填充率
  clk.drawRoundRect(0, 0, 44, 6, 3, 0xFFFF);       // 空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1, 1, huminum, 4, 2, humicol); // 实心圆角矩形
  clk.pushSprite(72, 222);                         // 窗口位置
  clk.deleteSprite();
}

// 温度图标显示函数
void tempWin()
{
  clk.setColorDepth(8);
  tempnum = tempnum + 10;
  clk.createSprite(66, 6);                         // 创建窗口
  clk.fillSprite(0x0000);                          // 填充率
  clk.drawRoundRect(0, 0, 66, 6, 3, 0xFFFF);       // 空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1, 1, tempnum, 4, 2, tempcol); // 实心圆角矩形
  clk.pushSprite(50, 192);                         // 窗口位置
  clk.deleteSprite();
}

#if DHT_EN

float DHT11_T = 0;
float DHT11_H = 0;

// 取数，在多任务调用。
bool NewDht = 0;
void getDHT11()
{
  DHT11_T = dht.readTemperature();
  DHT11_H = dht.readHumidity();
  NewDht = 1;
}

// 外接DHT11传感器，显示数据
void IndoorTem()
{
  if (!NewDht)
    return;
  NewDht = 0;
  float t = DHT11_T;
  float h = DHT11_H;
  // /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);
  tft.drawRoundRect(170, 112, 66, 48, 5, TFT_YELLOW); // 室内温湿度框
  // //位置
  // 温度
  clk.createSprite(60, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_CYAN, bgColor);
  clk.drawFloat(t, 1, 20, 13);
  clk.drawString("℃", 50, 13);
  clk.pushSprite(173, 112); // 184
  clk.deleteSprite();

  // 湿度
  clk.createSprite(60, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREENYELLOW, bgColor);
  clk.drawFloat(h, 1, 20, 13);
  clk.drawString("%", 50, 13);
  clk.pushSprite(173, 136); // 214
  clk.deleteSprite();
}
#endif

#if !WM_EN
// 微信配网函数
void SmartConfig(void)
{
  WiFi.mode(WIFI_STA); // 设置STA模式
  tft.pushImage(0, 0, 240, 240, qr);
  Serial.println("\r\nWait for Smartconfig..."); // 打印log信息
  WiFi.beginSmartConfig();                       // 开始SmartConfig，等待手机端发出用户名和密码
  while (1)
  {
    Serial.print(".");
    delay(100);                 // wait for a second
    if (WiFi.smartConfigDone()) // 配网成功，接收到SSID和密码
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
  loadNum = 194;
}
#endif

// 参考一下这位博主的函数输出函数
// 作者：济南凡事
// 链接：https://www.jianshu.com/p/7cef8e7b22d1
// 来源：简书
// 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
void ledcAnalogWrite(uint8_t channel, uint32_t value)
{

  uint32_t valueMax = 255;
  // calculate duty, 8191 from 2 ^ 13 - 1

  uint32_t duty = (8191 / valueMax) * min(value, valueMax);

  // write duty to LEDC

  ledcWrite(channel, duty);
}

String SMOD = ""; // 0亮度
bool isBlueT = 0;

// 串口调试设置函数
void Serial_set()
{
  String incomingByte = "";
  if (Serial.available() > 0)
  { // 增加蓝牙串口功能

    // 串口
    if (Serial.available() > 0)
    {
      while (Serial.available() > 0) // 监测串口缓存，当有数据输入时，循环赋值给incomingByte
      {
        if (iscntrl(Serial.peek()))
          Serial.read();
        else
          incomingByte += char(Serial.read()); // 读取单个字符值，转换为字符，并按顺序一个个赋值给incomingByte
        delay(2);                              // 不能省略，因为读取缓冲区数据需要时间
      }
      isBlueT = 0;
    }

    if (SMOD == "0x01") // 设置1亮度设置
    {
      int LCDBL = atoi(incomingByte.c_str()); // int n = atoi(xxx.c_str());//String转int
      if (LCDBL >= 0 && LCDBL <= 255)
      {
        EEPROM.write(BL_addr, LCDBL); // 亮度地址写入亮度值
        EEPROM.commit();              // 保存更改的数据
        delay(5);
        LCD_BL_PWM = EEPROM.read(BL_addr);
        delay(5);
        SMOD = "";
        Serial.printf("亮度调整为：");
        // analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
        ledcAnalogWrite(pwm_channel0, LCD_BL_PWM);
        Serial.println(LCD_BL_PWM);
        Serial.println("");
      }
      else
        Serial.println("亮度调整错误，请输入0-255");
    }
    if (SMOD == "0x02") // 设置2地址设置
    {
      unsigned int CityCODE = 0;
      unsigned int CityC = atoi(incomingByte.c_str());  // int n = atoi(xxx.c_str());//String转int
      CityC = (CityC == 101281001) ? 101281009 : CityC; // 湛江的代码改为霞山代码，解决湛江的代码取不到其他区的预警信号
      if (CityC >= 101000000 && CityC <= 102000000 || CityC == 0)
      {
        for (int cnum = 0; cnum < 5; cnum++)
        {
          EEPROM.write(CC_addr + cnum, CityC % 100); // 城市地址写入城市代码
          EEPROM.commit();                           // 保存更改的数据
          CityC = CityC / 100;
          delay(5);
        }
        for (int cnum = 5; cnum > 0; cnum--)
        {
          CityCODE = CityCODE * 100;
          CityCODE += EEPROM.read(CC_addr + cnum - 1);
          delay(5);
        }

        cityCode = CityCODE;

        if (cityCode == "0")
        {
          Serial.println("城市代码调整为：自动");
          getCityCode(); // 获取城市代码
        }
        else
        {
          Serial.printf("城市代码调整为：");
          Serial.println(cityCode);
        }
        UpdateWeater_en = 1;
        weatherWarn.config(HeUserKey, cityCode); // 配置请求信息  101230201厦门 101230201 厦门  101281006 湛江 101281009 霞山
        UpdateScreen = 1;
        // LCD_reflash(1); // 屏幕刷新程序
        SMOD = "";
      }
      else
        Serial.println("城市调整错误，请输入9位城市代码，自动获取请输入0");
    }
    if (SMOD == "0x03") // 设置3屏幕显示方向
    {
      int RoSet = atoi(incomingByte.c_str());
      if (RoSet >= 0 && RoSet <= 3)
      {
        EEPROM.write(Ro_addr, RoSet); // 屏幕方向地址写入方向值
        EEPROM.commit();              // 保存更改的数据
        SMOD = "";
        // 设置屏幕方向后重新刷屏并显示
        tft.setRotation(RoSet);
        tft.fillScreen(0x0000);
        UpdateWeater_en = 1;
        UpdateScreen = 1;
        // LCD_reflash(1); // 屏幕刷新程序
        // TJpgDec.drawJpg(15, 183, temperature, sizeof(temperature)); // 温度图标
        // TJpgDec.drawJpg(15, 213, humidity, sizeof(humidity));       // 湿度图标
        Serial.print("屏幕方向设置为：");
        Serial.println(RoSet);
      }
      else
      {
        Serial.println("屏幕方向值错误，请输入0-3内的值");
      }
    }
    if (SMOD == "0x04") // 设置天气更新时间
    {
      int wtup = atoi(incomingByte.c_str()); // int n = atoi(xxx.c_str());//String转int
      if (wtup >= 1 && wtup <= 60)
      {
        EEPROM.write(UpWeT_addr, wtup); // 更新时间地址写入更新时间值
        EEPROM.commit();                // 保存更改的数据
        delay(5);
        updateweater_time = EEPROM.read(UpWeT_addr);
        delay(5);
       // updateweater_time = wtup;
        SMOD = "";
        Serial.printf("天气更新时间更改为：");
        Serial.print(updateweater_time);
        Serial.println("分钟");
      }
      else
        Serial.println("更新时间太长，请重新设置（1-60）");
    }
    else
    {
      SMOD = incomingByte;
      delay(2);
      if (SMOD == "0x01")
        Serial.println("请输入亮度值，范围0-255");
      else if (SMOD == "0x02")
        Serial.println("请输入9位城市代码，自动获取请输入0");
      else if (SMOD == "0x03")
      {
        Serial.println("请输入屏幕方向值，");
        Serial.println("0-USB接口朝下");
        Serial.println("1-USB接口朝右");
        Serial.println("2-USB接口朝上");
        Serial.println("3-USB接口朝左");
      }
      else if (SMOD == "0x04")
      {
        Serial.print("当前天气更新时间：");
        Serial.print(updateweater_time);
        Serial.println("分钟");
        Serial.println("请输入天气更新时间（1-60）分钟");
      }
      else if (SMOD == "0x05")
      {
        Serial.println("重置WiFi设置中......");
        delay(10);
        wm.resetSettings();
        deletewificonfig();
        delay(10);
        Serial.println("重置WiFi成功");
        SMOD = "";
        ESP.restart();
      }
      else
      {
        Serial.println("");
        Serial.println("请输入需要修改的代码：");
        Serial.println("亮度设置输入        0x01");
        Serial.println("地址设置输入        0x02");
        Serial.println("屏幕方向设置输入    0x03");
        Serial.println("更改天气更新时间    0x04");
        Serial.println("重置WiFi(会重启)    0x05");
        Serial.println("");
      }
    }
  }
}

// 连接wifi后等待获取时间天气等信息显示窗口
void Wait_win(String showStr)
{
  clk.setColorDepth(8);
  clk.loadFont("msyhbd20", LittleFS);
  clk.createSprite(200, 100);                         // 创建窗口
  clk.fillSprite(0x0000);                             // 填充率
  clk.drawRoundRect(0, 0, 200, 16, 8, 0xFFFF);        // 空心圆角矩形
  clk.fillRoundRect(3, 3, loadNum, 10, 5, TFT_GREEN); // 实心圆角矩形
  clk.setTextDatum(CC_DATUM);                         // 设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("WiFi连接成功!!!", 100, 40, 2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawRightString("初始化...", 180, 60, 2);
  clk.pushSprite(20, 120); // 窗口位置
  clk.deleteSprite();

  clk.createSprite(200, 20);
  clk.fillSprite(0x0000);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawCentreString(showStr, 120, 0, 2);
  clk.pushSprite(20, 100);
  clk.deleteSprite();
  clk.unloadFont();
  loadNum += 1;
  if(loadNum >=255)
    loadNum = 0;
}

#if WM_EN
// WEB配网LCD显示函数
void Web_win()
{
  clk.setColorDepth(8);

  clk.createSprite(200, 60); // 创建窗口
  clk.fillSprite(0x0000);    // 填充率

  clk.setTextDatum(CC_DATUM); // 设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("WiFi Connect Fail!", 100, 10, 2);
  clk.drawString("SSID:", 45, 40, 2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawString("WeatherAP_XXXX", 125, 40, 2);
  clk.pushSprite(20, 50); // 窗口位置

  clk.deleteSprite();
}

// WEB配网函数
void Webconfig()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  delay(3000);
  wm.resetSettings(); // wipe settings

  // add a custom input field
  // int customFieldLength = 40;

  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

  // test custom html input type(checkbox)
  //  new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

  // test custom html(radio)
  // const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  // new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input

  const char *set_rotation = "<br/><label for='set_rotation'>Set Rotation</label>\
                              <input type='radio' name='set_rotation' value='0' checked> One<br>\
                              <input type='radio' name='set_rotation' value='1'> Two<br>\
                              <input type='radio' name='set_rotation' value='2'> Three<br>\
                              <input type='radio' name='set_rotation' value='3'> Four<br>";

  WiFiManagerParameter custom_rot(set_rotation); // custom html input
  WiFiManagerParameter custom_bl("LCDBL", "LCD BackLight(1-255)", "230", 3);
#if DHT_EN
  WiFiManagerParameter custom_DHT11("DHT11EN", "Enable DHT11 sensor", "1", 3);
#endif
  WiFiManagerParameter custom_weatertime("WeaterUpdateTime", "Weather Update Time(Min)(1-60)", "20", 3);
  WiFiManagerParameter custom_cc("CityCode", "CityCode", "101281001", 9); // 湛江101281001
  WiFiManagerParameter p_lineBreak_notext("<p></p>");

  // wm.addParameter(&p_lineBreak_notext);
  // wm.addParameter(&custom_field);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_cc);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_bl);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_weatertime);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_rot);
#if DHT_EN
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_DHT11);
#endif
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setSaveConfigCallback(SaveConfigCallback);
  // custom menu via array or vector
  //
  // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
  // const char* menu[] = {"wifi","info","param","sep","restart","exit"};
  // wm.setMenu(menu,6);
  std::vector<const char *> menu = {"wifi", "restart"};

  wm.setMenu(menu);

  // set dark theme
  wm.setClass("invert");

  // set static ip
  //  wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
  //  wm.setShowStaticFields(true); // force show static ip fields
  //  wm.setShowDnsFields(true);    // force show dns field always

  // wm.setConnectTimeout(20); // how long to try to connect for before continuing
  //  wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  wm.setMinimumSignalQuality(20); // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  uint64_t chipid;
  chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  String StrSSID = "WeatherAP_" + String((unsigned long)(uint16_t)chipid, HEX);

  res = wm.autoConnect(StrSSID.c_str()); // anonymous ap
                                         // res = wm.autoConnect("AutoConnectAP" + String((uint32_t)ESP.getEfuseMac())); // anonymous ap
  //  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  while (!res)
  {
    wm.process(); // avoid delays() in loop when non-blocking and other long running code
  }
}

String getParam(String name)
{
  // read parameter from server, for customhmtl input
  String value;
  if (wm.server->hasArg(name))
  {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback()
{
  int CCODE = 0, cc;

  Serial.println("[CALLBACK] saveParamCallback fired");

// 将从页面中获取的数据保存
#if DHT_EN
  DHT_img_flag = getParam("DHT11EN").toInt();
#endif
  updateweater_time = getParam("WeaterUpdateTime").toInt();
  cc = getParam("CityCode").toInt();
  LCD_Rotation = getParam("set_rotation").toInt();
  LCD_BL_PWM = getParam("LCDBL").toInt();

  // 对获取的数据进行处理
  // 城市代码
  Serial.print("CityCode = ");
  Serial.println(cc);
  if (cc >= 101000000 && cc <= 102000000 || cc == 0)
  {
    for (int cnum = 0; cnum < 5; cnum++)
    {
      EEPROM.write(CC_addr + cnum, cc % 100); // 城市地址写入城市代码
      EEPROM.commit();                        // 保存更改的数据
      cc = cc / 100;
      delay(5);
    }
    for (int cnum = 5; cnum > 0; cnum--)
    {
      CCODE = CCODE * 100;
      CCODE += EEPROM.read(CC_addr + cnum - 1);
      delay(5);
    }
    cityCode = CCODE;
  }
  // 屏幕方向
  Serial.print("LCD_Rotation = ");
  Serial.println(LCD_Rotation);
  if (EEPROM.read(Ro_addr) != LCD_Rotation)
  {
    EEPROM.write(Ro_addr, LCD_Rotation);
    EEPROM.commit();
    delay(5);
  }
  tft.setRotation(LCD_Rotation);
  tft.fillScreen(0x0000);
  Web_win();
  loadNum--;
  loading(1);
  if (EEPROM.read(BL_addr) != LCD_BL_PWM)
  {
    EEPROM.write(BL_addr, LCD_BL_PWM);
    EEPROM.commit();
    delay(5);
  }
  if (EEPROM.read(UpWeT_addr) != updateweater_time)
  {
    EEPROM.write(UpWeT_addr, updateweater_time);
    EEPROM.commit();
    delay(5);
  }
  // 屏幕亮度
  Serial.printf("亮度调整为：");
  ledcAnalogWrite(pwm_channel0, LCD_BL_PWM);
  Serial.println(LCD_BL_PWM);
  // 天气更新时间
  Serial.printf("天气更新时间调整为：");
  Serial.println(updateweater_time);

#if DHT_EN
  // 是否使用DHT11传感器
  Serial.printf("DHT11传感器：");
  EEPROM.write(DHT_addr, DHT_img_flag);
  EEPROM.commit(); // 保存更改的数据
  Serial.println((DHT_img_flag ? "已启用" : "未启用"));
#endif
}
//web配网成功后保存回调函数
void SaveConfigCallback(){
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("SSID:");
    Serial.println(WiFi.SSID().c_str());
    Serial.print("PSW:");
    Serial.println("************");
    strcpy(wificonf.stassid, WiFi.SSID().c_str()); // 名称复制
    strcpy(wificonf.stapsw, WiFi.psk().c_str());   // 密码复制
    savewificonfig();
    readwificonfig();
  }
}
#endif

bool haveNL = 0; // 每天凌晨更新农历
void LCD_reflash(bool en)
{
  // 更新天气情况和预警情况
  if (millis() - weaterTime > (60000 * updateweater_time) || UpdateWeater_en == 1)
  { // 20分钟更新一次天气
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WIFI已连接");
      getCityWeater();
      getWarning(); // 取当前预警
      if (UpdateWeater_en == 1)
        UpdateWeater_en = 0;
      weaterTime = millis();
    }
  }
  if (isNewWeather)
  {
    TJpgDec.drawJpg(31, 183, temperature, sizeof(temperature)); // 温度图标
    TJpgDec.drawJpg(50, 200, humidity, sizeof(humidity));       // 湿度图标
    weaterData();
    isNewWeather = false;
  }

  // 绘制时分秒
  if (now() != prevDisplay || en == 1)
  {
    prevDisplay = now();
    digitalClockDisplay(en);
  }

  // 绘制室内温度
#if DHT_EN
  if (DHT_img_flag != 0)
    IndoorTem();
#endif

  // 绘制太空人
  imgAnim();
  // 滚动显示天气情况和黄历
  switch (prevTime)
  {
  case 1:
    scrollDate();
    prevTime = 3;
    break;
  case 2:
    scrollBanner();
    prevTime = 4;
    break;
  default:
    break;
  }
  // 每天凌晨8分更新一下农历信息
  if ((rtc.getHour(true) == 0 && rtc.getMinute() == 8))
  {
    if (en == 1)
    {
      getNongli();
      haveNL = 1;
      return;
    }
    if (haveNL == 0)
    {
      getNongli();
      haveNL = 1;
    }
  }
  else
  {
    haveNL = 0;
  }

  /***********************************************************************************************************
   * 做一个双状态切换  1 -> 3
   *                    X
   *                 2  -> 4
   *    1显示scrollDate  2显示scrollBanner    显示完后：1切到3  2切到4   定时到后：3切到2  4切到1  实现定时切换轮播*
   ************************************************************************************************************/
  //  定时器计数 用来定时滚动显示 秒
  if (updateTime > 2)
  {
    updateTime = 0;

    switch (prevTime)
    {
    case 3:
      prevTime = 2;
      break;
    case 4:
      prevTime = 1;
      break;

    default:
      break;
    }
  }
  UpdateScreen = 0;
}

// 取得和风天气的预警信息
void getWarning()
{
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Error:WiFi is not Connected.");
    return;    
  }

  isNewWarn = false;
  scrollText[6] = "";

  if (weatherWarn.get())
  {
    if (weatherWarn.getStatus().equals("update") || weatherWarn.getStatus().equals("active"))
    {
      String T = weatherWarn.getTitle();
      int StrIndex = T.indexOf("布");
      int StrEnd = T.indexOf("信号");
      scrollText[6] = T.substring(StrIndex + 3, StrEnd);
      isNewWarn = true;
    }
    else
    {
      isNewWarn = false;
      scrollText[6] = "";
    }
  }
  else
  {
    Serial.println("Update Failed...");
    Serial.print("Server Response: ");
    Serial.println(weatherWarn.getServerCode());
    scrollText[6] = "";
    isNewWarn = false;
  }
}

// 显示天气预警界面
void DispWarn(int en)
{
  if (en)
  {
    String T = weatherWarn.getTitle();
    String X = weatherWarn.getWeatherText();

    if(T.isEmpty() || X.isEmpty()){
      Serial.println("warning is Empty!");
      return;      
    }

    int StrIndex = T.indexOf("布");
    String temp1 = T.substring(0, StrIndex - 3);
    String temp2 = T.substring(StrIndex + 3);
    String sColor = weatherWarn.getColor();

    if (sColor.equals("White"))
      tft.fillScreen(TFT_LIGHTGREY);
    else if (sColor.equals("Blue"))
      tft.fillScreen(TFT_BLUE);
    else if (sColor.equals("Green"))
      tft.fillScreen(TFT_GREEN);
    else if (sColor.equals("Yellow"))
      tft.fillScreen(TFT_YELLOW);
    else if (sColor.equals("Orange"))
      tft.fillScreen(TFT_ORANGE);
    else if (sColor.equals("Red"))
      tft.fillScreen(TFT_RED);
    else if (sColor.equals("Black"))
      tft.fillScreen(TFT_BLACK);
    else
      tft.fillScreen(TFT_SILVER);

    String typeFile = "/png/" + String(weatherWarn.getType(), DEC) + ".svg.jpg";
    TJpgDec.drawFsJpg(80, 10, typeFile, LittleFS);

    clk.setColorDepth(8);
    clk.loadFont("msyhbd20", LittleFS);
    clk.createSprite(220, 26 * 2);
    clk.fillSprite(TFT_PINK);
    clk.setTextWrap(false);
    clk.setTextDatum(TL_DATUM);
    clk.setTextColor(TFT_NAVY, TFT_PINK);

    clk.drawCentreString(temp2, 110, 3, 2);
    clk.drawCentreString(temp1, 110, 30, 2);

    clk.pushSprite(10, 162);
    clk.deleteSprite();

    clk.createSprite(230, 22 * 3);
    clk.fillSprite(TFT_WHITE);
    clk.setTextWrap(true);
    clk.setTextDatum(BL_DATUM);
    clk.setTextColor(TFT_BLACK, TFT_WHITE);

    int iCounter = 10 * X.length() / (12 * 2) - 30;

    for (int i = 0; i < iCounter; i++)
    {
      clk.fillSprite(TFT_WHITE);
      clk.drawString(X, 2, 60 - i * 2);
      clk.pushSprite(5, 90);

      // Continuous elliptical arc drawing
      fillArc(120, 120, inc * 6, 1, 120, 120, 5, rainbow(col));
      inc++;
      col += 1;
      if (col > 191)
        col = 0;
      if (inc > 59)
        inc = 0;

      delay(LOOP_DELAY);
    }

    // clk.pushSprite(10, 140);
    clk.deleteSprite();
    clk.unloadFont();

    matrix_effect.setTextAnimMode(AnimMode::SHOWCASE, "\n\r Weather Warning!!!       \n\r...气象警告!!!注意安全...        \n\r", 15, 120, 280);
    const long period = 22000;              // period at which to blink in ms
    unsigned long currentMillis = millis(); // store the current time
    tft.loadFont(ztqFont_20);
    do
    {
      matrix_effect.loop();
    } while (millis() - currentMillis <= period); // check if 1000ms passed
    tft.unloadFont();
    delay(10);
    isNewWeather = true;
    UpdateScreen = 1;

    tft.fillScreen(TFT_BLACK);
  }
  isNewWarn = false;
}

// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode()
{
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Error:WiFi is not Connected.");
    return;    
  }

  String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(rtc.getEpoch());
  // 创建 HTTPClient 对象
  HTTPClient httpClient; // 定义http客户端

  // 配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(wificlient, URL);

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");

  // 重试
  while (httpCode == -1 || httpCode == -11)
  {
    httpCode = httpClient.GET();
  }

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {
    String str = httpClient.getString();

    int aa = str.indexOf("id=");
    if (aa > -1)
    {
      unsigned int CityCODE = 0;
      // cityCode = str.substring(aa+4,aa+4+9).toInt();
      unsigned int CityC = str.substring(aa + 4, aa + 4 + 9).toInt();
      Serial.println("CityCode:" + cityCode);

      if (CityC >= 101000000 && CityC <= 102000000 || CityC == 0)
      {
        for (int cnum = 0; cnum < 5; cnum++)
        {
          EEPROM.write(CC_addr + cnum, CityC % 100); // 城市地址写入城市代码
          EEPROM.commit();                           // 保存更改的数据
          CityC = CityC / 100;
          delay(5);
        }
        for (int cnum = 5; cnum > 0; cnum--)
        {
          CityCODE = CityCODE * 100;
          CityCODE += EEPROM.read(CC_addr + cnum - 1);
          delay(5);
        }

        cityCode = CityCODE;
      }
      else
      {
        Serial.println("城市代码格式错误，获取城市代码失败");
      }
    }
    else
    {
      Serial.println("获取城市代码失败");
    }
  }
  else
  {
    Serial.println("请求城市代码错误：");
  }

  // 关闭ESP8266与服务器连接
  httpClient.end();
}

// 获取城市天气
void getCityWeater()
{
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Error:WiFi is not Connected.");
    return;    
  }

  String jsonCityDZ = "";
  String jsonDataSK = "";
  String jsonFC = "";

  String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(rtc.getEpoch());

  // 创建 HTTPClient 对象
  HTTPClient httpClient; // 定义http客户端

  httpClient.begin(URL);

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");

  // 重试
  while (httpCode == -1 || httpCode == -11)
  {
    httpCode = httpClient.GET();
    delay(1000);
  }

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {

    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");

    jsonCityDZ = str.substring(indexStart + 13, indexEnd);

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    jsonDataSK = str.substring(indexStart + 8, indexEnd);

    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    jsonFC = str.substring(indexStart + 5, indexEnd);

    isNewWeather = true;
    Serial.println("获取成功");

    // 解析第一段JSON
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonDataSK);
    JsonObject sk = doc.as<JsonObject>();

    tempnum = sk["temp"].as<int>();                                  // 温度
    huminum = atoi((sk["SD"].as<String>()).substring(0, 2).c_str()); // 湿度
    // 霞山的霞字在字库里面没有，所以调整为湛江 ：）
    cityname = sk["cityname"].as<String>();
    cityname = cityname.equals("霞山") ? "湛江" : cityname;

    pm25V = sk["aqi"];

    scrollText[0] = "实时天气 " + sk["weather"].as<String>();
    // scrollText[1] = "空气质量 " + aqiTxt;
    scrollText[2] = "风向 " + sk["WD"].as<String>() + sk["WS"].as<String>();
    Iconsname = atoi((sk["weathercode"].as<String>()).substring(1, 3).c_str());

    // 左上角滚动字幕
    // 解析第二段JSON
    deserializeJson(doc, jsonCityDZ);
    JsonObject dz = doc.as<JsonObject>();

    scrollText[3] = "今日" + dz["weather"].as<String>();

    deserializeJson(doc, jsonFC);
    JsonObject fc = doc.as<JsonObject>();

    scrollText[4] = "最低温度" + fc["fd"].as<String>() + "℃";
    scrollText[5] = "最高温度" + fc["fc"].as<String>() + "℃";
  }
  else
  {
    Serial.print("请求城市天气错误：");
    isNewWeather = true;
    jsonCityDZ = "{\"city\":\"湛江\",\"cityname\":\"zhanjiang\",\"temp\":\"999\",\"tempn\":\"29\",\"weather\":\"等待更新\",\"wd\":\"等待更新\",\"ws\":\"等待更新\",\"weathercode\":\"d1\",\"weathercoden\":\"n3\",\"fctime\":\"202306220800\"}";
    jsonDataSK = "{\"nameen\":\"zhanjiang\",\"cityname\":\"湛江\",\"city\":\"101281001\",\"temp\":\"00\",\"tempf\":\"00\",\"WD\":\"等待更新\",\"wde\":\"NW\",\"WS\":\"等待更新\",\"wse\":\"4km/h\",\"SD\":\"00%\",\"sd\":\"00%\",\"qy\":\"997\",\"njd\":\"13km\",\"time\":\"18:55\",\"rain\":\"0\",\"rain24h\":\"0\",\"aqi\":\"38\",\"aqi_pm25\":\"38\",\"weather\":\"等待更新\",\"weathere\":\"Cloudy\",\"weathercode\":\"d01\",\"limitnumber\":\"\",\"date\":\"等待更新\"}";
    jsonFC = "{\"fa\":\"01\",\"fb\":\"03\",\"fc\":\"34\",\"fd\":\"27\",\"fe\":\"等待更新\",\"ff\":\"等待更新\",\"fg\":\"等待更新\",\"fh\":\"等待更新\",\"fk\":\"5\",\"fl\":\"0\",\"fm\":\"999.9\",\"fn\":\"88.9\",\"fi\":\"6/22\",\"fj\":\"今天\"}";
  }
  // 关闭ESP8266与服务器连接
  httpClient.end();
}

//  获取农历信息
void getNongli()
{
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Error:WiFi is not Connected.");
    return;    
  }

  Serial.println("获取农历信息．．．");
  DynamicJsonDocument doc(1024);

  TotalHEAD = 3;
  scrolHEAD = new Display[TotalHEAD];

  if (scrolHEAD == NULL)
  {
    Serial.println("new scrolHEAD fail!!");
    return;
  }

  String Y = String(rtc.getYear());
  String M = rtc.getMonth() + 1 < 10 ? "0" + String(rtc.getMonth() + 1) : String(rtc.getMonth() + 1);
  String D = rtc.getDay() < 10 ? "0" + String(rtc.getDay()) : String(rtc.getDay());

  memset(scrolHEAD, '\0', TotalHEAD);

  scrolHEAD[0].title = monthDay() + " " + week();
  scrolHEAD[0].color = cWHITE;

  // https://www.mxnzp.com/api/holiday/single/20181121?ignoreHoliday=false&app_id=不再提供请自主申请&app_secret=不再提供请自主申请
  String URL = "https://www.mxnzp.com/api/holiday/single/" + Y + M + D + "?ignoreHoliday=false&app_id=" + mx_id + "&app_secret=" + mx_secret;

  // 创建 HTTPClient 对象
  HTTPClient httpClient; // 定义http客户端

  WiFiClientSecure *client = new WiFiClientSecure;

  if (!client)
  {
    Serial.println("Unable to create client");
    return;
  }

  client->setInsecure();
  httpClient.begin(*client, URL);

  const char *headerKeys[] = {"Connection", "Content-Encoding", "Content-Type", "Date", "Server", "Transfer-Encoding", "Vary"};
  const size_t numberOfHeaders = 7;
  httpClient.collectHeaders(headerKeys, numberOfHeaders);
  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/114.0");
  httpClient.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
  httpClient.addHeader("Accept-Encoding", "gzip, deflate, br");
  httpClient.addHeader("Accept-Language", "zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2");
  httpClient.addHeader("Host", "https://www.mxnzp.com/");
  httpClient.addHeader("Connection", "keep-alive");
  httpClient.addHeader("Sec-Fetch-Dest", "document");
  httpClient.addHeader("Sec-Fetch-Mode", "navigate");
  httpClient.addHeader("Sec-Fetch-Site", "none");
  httpClient.addHeader("Sec-Fetch-User", "?1");
  httpClient.addHeader("Upgrade-Insecure-Requests", "1");

  // 重试次数
  int httpCode = 0;
  int iRetry = 1;

  do
  {
    // 启动连接并发送HTTP请求
    httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_OK)
    {
      break;
    }
    else
    {
      delay(3000);
    }
    iRetry++;
  } while (!(httpCode == HTTP_CODE_OK) && iRetry < 10);

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {
    String response = "";
    int content_len = httpClient.getSize(); // get length of document (is -1 when Server sends no Content-Length header)
    if ((httpClient.hasHeader("Content-Encoding")) && (httpClient.header("Content-Encoding").equals("gzip")))
    {
      Serial.println("httpget() ok");
      WiFiClient *streamptr = httpClient.getStreamPtr();
      Serial.println("getStreamPtr() ok");
      // Gzip解压
      if (streamptr != nullptr)
      {
        Serial.println("streamptr ok");
        TarGzUnpacker *TARGZUnpacker = new TarGzUnpacker();
        Serial.println("TARGZUnpacker ok");

        TARGZUnpacker->haltOnError(true);                                                            // stop on fail (manual restart/reset required)
        TARGZUnpacker->setTarVerify(true);                                                           // true = enables health checks but slows down the overall process
        TARGZUnpacker->setupFSCallbacks(targzTotalBytesFn, targzFreeBytesFn);                        // prevent the partition from exploding, recommended
        TARGZUnpacker->setGzProgressCallback(BaseUnpacker::defaultProgressCallback);                 // targzNullProgressCallback or defaultProgressCallback
        TARGZUnpacker->setLoggerCallback(BaseUnpacker::targzPrintLoggerCallback);                    // gz log verbosity
        TARGZUnpacker->setTarProgressCallback(BaseUnpacker::defaultProgressCallback);                // prints the untarring progress for each individual file
        TARGZUnpacker->setTarStatusProgressCallback(BaseUnpacker::defaultTarStatusProgressCallback); // print the filenames as they're expanded
        TARGZUnpacker->setTarMessageCallback(BaseUnpacker::targzPrintLoggerCallback);                // tar log verbosity

        Serial.println("settings ok");

        if (!TARGZUnpacker->tarGzStreamExpander(streamptr, tarGzFS))
        {
          Serial.printf("tarGzStreamExpander failed with return code #%d\n", TARGZUnpacker->tarGzGetError());
        }
        else
        {
          // print leftover bytes if any (probably zero-fill from the server)
          while (httpClient.connected())
          {
            size_t streamSize = streamptr->available();
            if (streamSize)
            {
              response += streamptr->read();
            }
            else
              break;
          }
        }
        delete TARGZUnpacker;
      }
      else
      {
        Serial.println("Failed to establish http connection");
      }
    }
    else
    {
      response = httpClient.getString();
    }
    // http结束

    // 开始JSON解析
    //  反序列化JSON
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonObject root = doc.as<JsonObject>();
    JsonObject data = root["data"];

    const char *suit = data["suit"];   // 今日宜
    const char *avoid = data["avoid"]; // 今日忌

    scrolHEAD[1].title = data["yearTips"].as<String>() + "年 " + data["lunarCalendar"].as<String>();
    scrolHEAD[1].color = cWHITE;
    scrolHEAD[2].title = data["chineseZodiac"].as<String>() + "年" + data["weekOfYear"].as<String>() + "周" + " " + data["typeDes"].as<String>();
    scrolHEAD[2].color = cWHITE;

    // 宜忌有时描述太长，分行显示
    String *rword = NULL;
    int needle = 0;

    rword = new String[strlen(suit)];
    if (rword == NULL)
    {
      Serial.println("new rword fail!!");
      return;
    }
    memset(rword, '\0', sizeof(rword));

    // temp = suit;
    if (strlen(suit) > 0)
    {
      TotalYI = StrSplit(suit, ".", rword);

      scrollYI = new Display[TotalYI];
      if (scrollYI == NULL)
      {
        Serial.println("new scrollYI fail!!");
        return;
      }
      memset(scrollYI, '\0', TotalYI);

      if (TotalYI < MaxScroll)
      {
        for (int i = 0; i < TotalYI; i += 2)
        {
          scrollYI[needle].title = "宜:" + rword[i] + " " + rword[i + 1];
          scrollYI[needle].color = cGREEN;
          needle++;
          if (needle > MaxScroll)
            break;
        }
      }
      else
      {
        Serial.println("suit字符串太大。");
      }
    }
    else
    {
      scrollYI[needle].title = "宜:";
      scrollYI[needle].color = cWHITE;
      needle++;
    }
    TotalYI = needle;
    needle = 0;

    if (rword != NULL)
      delete[] rword;

    rword = new String[strlen(avoid)];
    if (rword == NULL)
    {
      Serial.println("new rword fail!!");
      return;
    }
    memset(rword, '\0', sizeof(rword));
    if (strlen(avoid) > 0)
    {
      TotalJI = StrSplit(avoid, ".", rword);

      scrollJI = new Display[TotalJI];
      if (scrollJI == NULL)
      {
        Serial.println("new scrollJI fail!!");
        return;
      }
      memset(scrollJI, '\0', TotalJI);

      if (TotalJI < MaxScroll)
      {
        for (int i = 0; i < TotalJI; i += 2)
        {
          scrollJI[needle].title = "忌:" + rword[i] + " " + rword[i + 1];
          scrollJI[needle].color = cRED;
          needle++;
          if (needle > MaxScroll)
            break;
        }
      }
      else
      {
        Serial.println("avoid字符串太大。");
      }
    }
    else
    {
      scrollJI[needle].title = "忌:";
      scrollJI[needle].color = cRED;
      needle++;
    }
    TotalJI = needle;
    if (rword != NULL)
      delete[] rword;

    if (scrollNongLi != NULL)
      delete[] scrollNongLi;

    TotalDis = TotalHEAD + TotalYI + TotalJI;
    scrollNongLi = new Display[TotalDis];
    if (scrollNongLi == NULL)
    {
      Serial.println("new scrollNongLi fail!!");
      return;
    }
    memset(scrollNongLi, '\0', TotalDis);

    my_strcat_arrcy(scrolHEAD, TotalHEAD, scrollYI, TotalYI, scrollJI, TotalJI, scrollNongLi);

    if (scrollJI != NULL)
      delete[] scrollJI;
    if (scrollYI != NULL)
      delete[] scrollYI;
  }
  else
  {
    if (scrollNongLi != NULL)
      delete[] scrollNongLi;

    TotalDis = TotalHEAD;
    scrollNongLi = new Display[TotalDis];
    if (scrollNongLi == NULL)
    {
      Serial.println("new scrollNongLi fail!!");
      return;
    }
    memset(scrollNongLi, '\0', TotalDis);

    for (int i = 0; i < TotalDis; i++)
    {
      scrollNongLi[i] = scrolHEAD[i];
    }

    Serial.print("请求农历信息错误：");
  }

  if (scrolHEAD != NULL)
    delete[] scrolHEAD;

  // 关闭ESP8266与服务器连接
  httpClient.end();
  delete client;
}

/**
 * 功能：HTTPS请求封装！
 * @param host：请求域名（String类型）
 * @param url：请求地址（String类型）
 * @param parameter：请求参数(String类型)(默认""")
 * @param fingerprint：服务器证书指纹 (String类型)(默认""")
 * @param Port：请求端口(int类型)(默认：443)
 * @param Receive_cache：接收缓存(int类型)(默认：1024)
 * @return 成功返回请求的内容(String类型) 失败则返回"0"
 * */
String HTTPS_request(String host, String url, String parameter)
{
  // WiFiClientSecure HTTPS; //建立WiFiClientSecure对象
  WiFiClientSecure *HTTPS; // 网络连接对象。
  delete HTTPS;
  HTTPS = new WiFiClientSecure;
  uint16_t httpsPort = 443; // 连接端口号。
  int32_t TimerOut = 15000;
  uint8_t writeBuffer[500];
  uint16_t Index;
  const uint16_t BufferSize = 8192; // 缓存大小。缓存足够才能保证在WIFI接收间隔期间不会中断。不会有停顿或跳跃感。
  uint16_t BufferIndex;             // 缓存指针。

  if (parameter != "")
    parameter = "?" + parameter;
  String postRequest = (String)("GET ") + url + parameter + " HTTP/1.1\r\n" +
                       "Host: " + host + "\r\n" +
                       "User-Agent: Mozilla/5.0 (iPhone; CPU iPhone OS 13_2_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.3 Mobile/15E148 Safari/604.1 Edg/103.0.5060.53" +
                       "\r\n\r\n";
  HTTPS->setInsecure();        // 不进行服务器身份认证
  HTTPS->setTimeout(TimerOut); // 设置超时。
  int cache = sizeof(postRequest) + 10;
  HTTPS->setTimeout(15000); // 设置等待的最大毫秒数
  Serial.println("初始化参数完毕！\n开始连接服务器==>>>>>");
  HTTPS->connect(host.c_str(), httpsPort, TimerOut);

  Index = 100;
  while (!HTTPS->connected() && Index > 0)
  {
    delay(100);
    Index--;
  }
  if (!HTTPS->connected())
  {
    Serial.println("Connection failed");
    return "failed";
  }
  else
  {
    Serial.println(" connected");
  } // 超过10秒未建立连接则显示连接失败并退出。
  for (Index = 0; Index < postRequest.length(); Index++)
  {
    writeBuffer[Index] = postRequest[Index];
  }
  HTTPS->write(writeBuffer, Index);

  Index = 100;
  BufferIndex = 0;
  postRequest = "";
  while (Index)
  {
    delay(100);
    Index--;
    while (HTTPS->available())
    {
      postRequest = postRequest + char(HTTPS->read());
      BufferIndex++;
      if (BufferIndex > 30)
      {
        // 等待服务器HTTP回应信息。
        if (postRequest[BufferIndex - 1] == '\n' && postRequest[BufferIndex - 2] == '\r' && postRequest[BufferIndex - 3] == '\n' && postRequest[BufferIndex - 4] == '\r')
        {
          Index = 0;
          break;
        }
      }
      if (BufferIndex > BufferSize)
      {
        Index = 0;
        break;
      } // 非HTTP响应，放弃并跳出。
    }
  }

  String line;
  while (HTTPS->connected())
  {
    line = HTTPS->readStringUntil('\n');
    if (line.length() > 10)
      break;
  }
  HTTPS->stop(); // 操作结束，断开服务器连接
  delay(500);
  return line;
}

// 3个数组相加出来1个
void my_strcat_arrcy(Display *arr, int lena, Display *brr, int lenb, Display *crr, int lenc, Display *str)
{
  int i = 0, j = 0;
  for (i = 0; i < lena; i++)
  {
    str[j] = arr[i];
    j++;
  }
  for (i = 0; i < lenb; i++)
  {
    str[j] = brr[i];
    j++;
  }
  for (i = 0; i < lenc; i++)
  {
    str[j] = crr[i];
    j++;
  }
}
// gzip解压进度
void myTarProgressCallback(uint8_t progress)
{
  static int8_t myLastProgress = -1;
  if (myLastProgress != progress)
  {
    myLastProgress = progress;
    if (progress == 0)
    {
      Serial.print("Progress: [0% ");
    }
    else if (progress == 100)
    {
      Serial.println(" 100%]\n");
    }
    else
    {
      switch (progress)
      {
      case 25:
        Serial.print(" 25% ");
        break;
      case 50:
        Serial.print(" 50% ");
        break;
      case 75:
        Serial.print(" 75% ");
        break;
      default:
        Serial.print("T");
        break;
      }
    }
  }
}
// 天气信息写到屏幕上
void weaterData()
{

  /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20); // ZdyLwFont_20

  // 温度
  clk.createSprite(58, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  // clk.drawString(sk["temp"].as<String>() + "℃", 28, 13);
  clk.drawString(String(tempnum, DEC) + "℃", 28, 13);
  clk.pushSprite(110, 184);
  clk.deleteSprite();
  // tempnum = sk["temp"].as<int>();
  tempnum = tempnum + 10;
  if (tempnum < 10)
    tempcol = 0x00FF;
  else if (tempnum < 28)
    tempcol = 0x0AFF;
  else if (tempnum < 34)
    tempcol = 0x0F0F;
  else if (tempnum < 41)
    tempcol = 0xFF0F;
  else if (tempnum < 49)
    tempcol = 0xF00F;
  else
  {
    tempcol = 0xF00F;
    tempnum = 50;
  }
  tempWin();

  // 湿度
  clk.createSprite(58, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(String(huminum, DEC)+'%', 28, 13);
  // clk.drawString("100%",28,13);
  clk.pushSprite(110, 214);
  clk.deleteSprite();
  // String A = sk["SD"].as<String>();
  // huminum = atoi((sk["SD"].as<String>()).substring(0, 2).c_str());

  if (huminum > 90)
    humicol = 0x00FF;
  else if (huminum > 70)
    humicol = 0x0AFF;
  else if (huminum > 40)
    humicol = 0x0F0F;
  else if (huminum > 20)
    humicol = 0xFF0F;
  else
    humicol = 0xF00F;
  humidityWin();

  // 城市名称
  clk.createSprite(94, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  // 霞山的霞字在字库里面没有，所以调整为湛江 ：）
  // String cityname = sk["cityname"].as<String>();
  // cityname = cityname.equals("霞山") ? "湛江" : cityname;
  clk.drawString(cityname, 44, 16);
  clk.pushSprite(26, 15); // 15,15
  clk.deleteSprite();

  // PM2.5空气指数
  uint16_t pm25BgColor = tft.color565(156, 202, 127); // 优
  String aqiTxt = "优";
  // int pm25V = sk["aqi"];
  if (pm25V > 200)
  {
    pm25BgColor = tft.color565(136, 11, 32); // 重度
    aqiTxt = "重度";
  }
  else if (pm25V > 150)
  {
    pm25BgColor = tft.color565(186, 55, 121); // 中度
    aqiTxt = "中度";
  }
  else if (pm25V > 100)
  {
    pm25BgColor = tft.color565(242, 159, 57); // 轻
    aqiTxt = "轻度";
  }
  else if (pm25V > 50)
  {
    pm25BgColor = tft.color565(247, 219, 100); // 良
    aqiTxt = "良";
  }
  clk.createSprite(56, 24);
  clk.fillSprite(bgColor);
  clk.fillRoundRect(0, 0, 50, 24, 4, pm25BgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(0x0000);
  clk.drawString(aqiTxt, 25, 13);
  clk.pushSprite(104, 18);
  clk.deleteSprite();

  // scrollText[0] = "实时天气 " + sk["weather"].as<String>();
  scrollText[1] = "空气质量 " + aqiTxt;
  // scrollText[2] = "风向 " + sk["WD"].as<String>() + sk["WS"].as<String>();

  // 天气图标  170,15
  wrat.printfweather(160, 15, Iconsname);

  clk.unloadFont();
}

// 滚动显示
int currentIndex = 0;
void scrollBanner()
{
  if (scrollText[currentIndex] != "")
  {
    clk.setColorDepth(8);
    scrollText[6].isEmpty() ? clk.loadFont(ZdyLwFont_20) : clk.loadFont("msyhbd20", LittleFS);

    clk.createSprite(150, 30);
    clk.fillSprite(bgColor);
    clk.setTextWrap(false);
    clk.setTextDatum(CC_DATUM);
    scrollText[6].isEmpty() ? clk.setTextColor(TFT_WHITE, bgColor) : clk.setTextColor(TFT_MAGENTA, bgColor);

    clk.drawString(scrollText[currentIndex], 74, 16);
    clk.pushSprite(10, 45);

    clk.deleteSprite();
    clk.unloadFont();
  }
  if (currentIndex >= 6)
    currentIndex = 0; // 回第一个
  else
    currentIndex += 1; // 准备切换到下一个
}

void scrollDate()
{
  if (scrollNongLi == NULL)
  {
    Serial.println("scrollNongLi is NULL");
    return;
  }
  if (scrollNongLi[CurrentDisDate].title && CurrentDisDate < TotalDis)
  {
    if (scrollNongLi[CurrentDisDate].title != "")
    {

      /***日期****/
      clk.setColorDepth(8);

      // 新的装载字库的方法：
      // 直接先用processing生成xxx.vlw 格式的文件
      // 把xxx.vlw放在platfomio项目下创建的data目录
      // 把vlw文件上传到单片机的flash空间中
      // 然后在这里调用
      clk.loadFont("msyhbd20", LittleFS);
      // 星期
      clk.createSprite(162, 30);
      clk.fillSprite(bgColor);
      clk.setTextDatum(CC_DATUM);
      switch (scrollNongLi[CurrentDisDate].color)
      {
      case cRED:
        clk.setTextColor(0xFDB8, bgColor);
        /* code */
        break;
      case cGREEN:
        clk.setTextColor(TFT_GREEN, bgColor);
        /* code */
        break;
      case cWHITE:
        clk.setTextColor(TFT_WHITE, bgColor);
        /* code */
        break;

      default:
        break;
      }
      clk.drawString(scrollNongLi[CurrentDisDate].title, 75, 16);
      clk.pushSprite(10, 150);
      clk.deleteSprite();
      clk.unloadFont();
    }
    else
    {
      CurrentDisDate = 0;
      return;
    }

    if (CurrentDisDate >= TotalDis - 1)
    {
      CurrentDisDate = 0; // 回第一个
      return;
    }
    else
    {
      CurrentDisDate += 1; // 准备切换到下一个
      return;
    }
  }
  else
  {
    CurrentDisDate = 0; // 回第一个
    return;
  }
}

#if imgAst_EN
void imgAnim()
{
  int x = 160, y = 160;
  if (millis() - AprevTime > 37) // x ms切换一次
  {
    Anim++;
    AprevTime = millis();
  }
  if (Anim == 10)
    Anim = 0;

  switch (Anim)
  {
  case 0:
    TJpgDec.drawJpg(x, y, i0, sizeof(i0));
    break;
  case 1:
    TJpgDec.drawJpg(x, y, i1, sizeof(i1));
    break;
  case 2:
    TJpgDec.drawJpg(x, y, i2, sizeof(i2));
    break;
  case 3:
    TJpgDec.drawJpg(x, y, i3, sizeof(i3));
    break;
  case 4:
    TJpgDec.drawJpg(x, y, i4, sizeof(i4));
    break;
  case 5:
    TJpgDec.drawJpg(x, y, i5, sizeof(i5));
    break;
  case 6:
    TJpgDec.drawJpg(x, y, i6, sizeof(i6));
    break;
  case 7:
    TJpgDec.drawJpg(x, y, i7, sizeof(i7));
    break;
  case 8:
    TJpgDec.drawJpg(x, y, i8, sizeof(i8));
    break;
  case 9:
    TJpgDec.drawJpg(x, y, i9, sizeof(i9));
    break;
  default:
    Serial.println("显示Anim错误");
    break;
  }
}
#endif

unsigned char Hour_sign = 60;
unsigned char Minute_sign = 60;
unsigned char Second_sign = 60;
void digitalClockDisplay(int reflash_en)
{
  int timey = 82;
  if (rtc.getHour(true) != Hour_sign || reflash_en == 1) // 时钟刷新
  {
    dig.printfW3660(20 - 10, timey, rtc.getHour(true) / 10);
    dig.printfW3660(60 - 10, timey, rtc.getHour(true) % 10);
    Hour_sign = rtc.getHour(true);
  }
  if (rtc.getMinute() != Minute_sign || reflash_en == 1) // 分钟刷新
  {
    dig.printfO3660(101 - 10, timey, rtc.getMinute() / 10);
    dig.printfO3660(141 - 10, timey, rtc.getMinute() % 10);
    Minute_sign = rtc.getMinute();
  }
  if (rtc.getSecond() != Second_sign || reflash_en == 1) // 秒钟刷新
  {

    if (DHT_img_flag == 1){
      dig.printfW1830(182 - 10, timey, rtc.getSecond() / 10);
      dig.printfW1830(202 - 10, timey, rtc.getSecond() % 10);      
    }else{
      dig.printfW1830(182 - 10, timey + 30, rtc.getSecond() / 10);
      dig.printfW1830(202 - 10, timey + 30, rtc.getSecond() % 10);         
    }

    Second_sign = rtc.getSecond();
  }

  if (reflash_en == 1)
    reflash_en = 0;
}

// 星期
String week()
{
  String wk[7] = {"日", "一", "二", "三", "四", "五", "六"};
  String s = "周" + wk[rtc.getDayofWeek()];
  return s;
}

// 月日
String monthDay()
{
  String s = String(rtc.getMonth() + 1);
  s = s + "月" + rtc.getDay() + "日";
  return s;
}

/*-------- NTP code ----------*/

time_t getNtpTime()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    timeClient.update();
    rtc.setTime(timeClient.getEpochTime());
    return timeClient.getEpochTime();
  }
  return 0;
}

/* **************************************************************************
/* *******************设置显示屏休眠时间***************************************
/* **************************************************************************/

int sethourK = 21; // 几点比如输入01表示1点，13表示13点
int setminK = 00;  // 几分开比如输入05表示5分，22表示22分

int sethourG = 8; // 几点关
int setminG = 30; // 几分关
bool isSleepMode = 0;
int bLight = LCD_BL_PWM;
uint32_t preTime;

/**
 * @brief 设置睡眠时间
 *
 * @param data
 */
void sleepTimeLoop(uint8_t Maxlight, uint8_t Minlight)
{
  uint32_t starttime = sethourK * 100 + setminK; // 开始时间
  uint32_t endtime = sethourG * 100 + setminG;   // 结束时间

  if (starttime == endtime) // 如果开始时间和结束时间是一样的话,就什么都不做
  {
    return;
  }

  uint32_t currtime = rtc.getHour(true) * 100 + rtc.getMinute(); // 当前时间
  if (currtime == preTime)
  {
    return;
  }

  preTime = currtime;
  if (starttime < endtime) // 如果开始时间小于结束时间,则只需要判断当前时间是否在开始时间和结束时间的区间范围内
  {
    if (currtime >= starttime && currtime < endtime) // 如果时间在休眠时间范围内则休眠
    {
      if (!isSleepMode)
      {
        isSleepMode = true; // 标记进入睡眠模式
        bLight = Minlight;  // 调小LED背光
      }
    }
    else
    {
      if (isSleepMode)
      {
        // 这里避免出现误操作,每次都将屏幕点亮,将屏幕亮度设置到预设亮度
        isSleepMode = false; // 标记退出睡眠模式
        bLight = Maxlight;   // 调大LED背光
      }
    }
  }
  else // 如果开始时间大于结束时间,表示表示当前时间在反向的范围内则不需要休眠
  {
    if (currtime >= endtime && currtime < starttime) // 如果时间在休眠时间范围内则休眠
    {
      if (isSleepMode)
      {
        // 这里避免出现误操作,每次都将屏幕点亮,将屏幕亮度设置到预设亮度
        isSleepMode = false; // 标记退出睡眠模式
        bLight = Maxlight;   // 调大LED背光
      }
    }
    else
    {
      if (!isSleepMode)
      {
        isSleepMode = true; // 标记进入睡眠模式
        bLight = Minlight;  // 调小LED背光
      }
    }
  }
  ledcAnalogWrite(pwm_channel0, bLight); // 写入
}

// 创建一个函数，用来根据一个分割符来分割一个字符串
// 入口参数（目标字符串，分割符，返回的结果字符串数组） 返回分割数量
int StrSplit(String str, String fen, String *result)
{
  int index = 0, i = 0;

  if (str.length() == 0)
    return 0;
  do
  {
    index = str.indexOf(fen);
    if (index != -1)
    {
      result[i] = str.substring(0, index);
      str = str.substring(index + fen.length(), str.length());
      i++;
    }
    else
    {
      if (str.length() > 0)
        result[i] = str;
      i++;
    }
  } while (index >= 0);
  // 保证为偶数，便于后面显示
  if (i % 2 != 0)
    result[i] = "";
  return i;
}

// #########################################################################
// Draw a circular or elliptical arc with a defined thickness
// #########################################################################

// x,y == coords of centre of arc
// start_angle = 0 - 359
// seg_count = number of 6 degree segments to draw (60 => 360 degree arc)
// rx = x axis outer radius
// ry = y axis outer radius
// w  = width (thickness) of arc in pixels
// colour = 16 bit colour value
// Note if rx and ry are the same then an arc of a circle is drawn

void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{

  byte seg = 6; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc)
  {

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

    // Copy segment end to sgement start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;
  }
}

// #########################################################################
// Return the 16 bit colour with brightness 0-100%
// #########################################################################
unsigned int brightness(unsigned int colour, int brightness)
{
  byte red = colour >> 11;
  byte green = (colour & 0x7E0) >> 5;
  byte blue = colour & 0x1F;

  blue = (blue * brightness) / 100;
  green = (green * brightness) / 100;
  red = (red * brightness) / 100;

  return (red << 11) + (green << 5) + blue;
}

// #########################################################################
// Return a 16 bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  switch (state)
  {
  case 0:
    green++;
    if (green == 64)
    {
      green = 63;
      state = 1;
    }
    break;
  case 1:
    red--;
    if (red == 255)
    {
      red = 0;
      state = 2;
    }
    break;
  case 2:
    blue++;
    if (blue == 32)
    {
      blue = 31;
      state = 3;
    }
    break;
  case 3:
    green--;
    if (green == 255)
    {
      green = 0;
      state = 4;
    }
    break;
  case 4:
    red++;
    if (red == 32)
    {
      red = 31;
      state = 5;
    }
    break;
  case 5:
    blue--;
    if (blue == 255)
    {
      blue = 0;
      state = 0;
    }
    break;
  }
  return red << 11 | green << 5 | blue;
}
#if WebSever_EN
// web网站相关函数
// web设置页面
void handleconfig()
{
  String msg = "Ready...";
  int web_cc, web_setro, web_lcdbl, web_upt, web_dhten;

  if (server.hasArg("web_ccode") || server.hasArg("web_bl") ||
      server.hasArg("web_upwe_t") || server.hasArg("web_DHT11_en") || server.hasArg("web_set_rotation"))
  {
    web_cc = server.arg("web_ccode").toInt();
    web_setro = server.arg("web_set_rotation").toInt();
    web_lcdbl = server.arg("web_bl").toInt();
    web_upt = server.arg("web_upwe_t").toInt();
    web_dhten = server.arg("web_DHT11_en").toInt();
    Serial.println("");
    if (web_cc >= 101000000 && web_cc <= 102000000)
    {
      saveCityCodetoEEP(&web_cc);
      readCityCodefromEEP(&web_cc);
      cityCode = web_cc;
      Serial.print("城市代码:");
      Serial.println(web_cc);
      UpdateWeater_en = 1;
      weatherWarn.config(HeUserKey, cityCode); // 配置请求信息  101230201厦门 101230201 厦门  101281006 湛江 101281009 霞山
      UpdateScreen = 1;
      msg = "Sent OK!!!";
    }
    if (web_lcdbl > 0 && web_lcdbl <= 255)
    {
      EEPROM.write(BL_addr, web_lcdbl); // 亮度地址写入亮度值
      EEPROM.commit();                  // 保存更改的数据
      delay(5);
      LCD_BL_PWM = EEPROM.read(BL_addr);
      delay(5);
      Serial.printf("亮度调整为：");
      // analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM * 10));
      ledcAnalogWrite(pwm_channel0, LCD_BL_PWM);
      Serial.println(LCD_BL_PWM);
      Serial.println("");
      msg = "Sent OK!!!";
    }
    if (web_upt > 0 && web_upt <= 60)
    {
      EEPROM.write(UpWeT_addr, web_upt); // 亮度地址写入亮度值
      EEPROM.commit();                   // 保存更改的数据
      delay(5);
      updateweater_time = EEPROM.read(UpWeT_addr);
      delay(5);
      //updateweater_time = web_upt;
      Serial.print("天气更新时间（分钟）:");
      Serial.println(web_upt);
      msg = "Sent OK!!!";
    }

    EEPROM.write(DHT_addr, web_dhten);
    EEPROM.commit(); // 保存更改的数据
    delay(5);
    if (web_dhten != DHT_img_flag)
    {
      DHT_img_flag = web_dhten;
      tft.fillScreen(0x0000);
      // LCD_reflash(1); // 屏幕刷新程序
      UpdateScreen = 1;
      UpdateWeater_en = 1;
      msg = "Sent OK!!!";
      // TJpgDec.drawJpg(15, 183, temperature, sizeof(temperature)); // 温度图标
      // TJpgDec.drawJpg(15, 213, humidity, sizeof(humidity));       // 湿度图标
    }
    Serial.print("DHT Sensor Enable： ");
    Serial.println(DHT_img_flag);

    EEPROM.write(Ro_addr, web_setro);
    EEPROM.commit(); // 保存更改的数据
    delay(5);
    if (web_setro != LCD_Rotation)
    {
      LCD_Rotation = web_setro;
      tft.setRotation(LCD_Rotation);
      tft.fillScreen(0x0000);
      UpdateWeater_en = 1;
      UpdateScreen = 1;
      msg = "Sent OK!!!";
      //   // LCD_reflash(1); // 屏幕刷新程序
      // LCD_reflash(1); // 屏幕刷新程序
      // UpdateWeater_en = 1;
      TJpgDec.drawJpg(15, 183, temperature, sizeof(temperature)); // 温度图标
      TJpgDec.drawJpg(15, 213, humidity, sizeof(humidity));       // 湿度图标
    }
    Serial.print("LCD Rotation:");
    Serial.println(LCD_Rotation);
  }

  // 网页界面代码段
  String content = "<html><style>html,body{ background: #1aceff; color: #fff; font-size: 10px;}</style>";
  content += "<body><form action='/' method='POST'><br><div>SDD Web Config</div><br>";
  content += "City Code:<br><input type='text' list='fruits' id='cityid' onclick='clearinput()' name='web_ccode' placeholder='city code' value='" + cityCode + "'><br>";
  content += "<datalist id='fruits'>\
              <option value='101281001'>\
              <option value='101281002'>\
              <option value='101281003'>\
              <option value='101281004'>\
              <option value='101281005'>\
              <option value='101281006'>\
              <option value='101281007'>\
              <option value='101281008'>\
              <option value='101281009'>\
              <option value='101010100'>\
              <option value='101020100'>\
              <option value='101230201'>\
              <option value='101280101'>\
              <option value='101250101'>\
              <option value='101110101'>\
              </datalist>";
  content += "<br>Back Light(1-255):(default:50)<br><input type='text' name='web_bl' placeholder='50' value='" + String(LCD_BL_PWM, DEC) + "'><br>";
  content += "<br>Weather Update(1-60) Time:(default:10)<br><input type='text' name='web_upwe_t' placeholder='10' value= '" + String(updateweater_time, DEC) + "'><br>";
#if DHT_EN
  content += "<br>DHT Sensor Enable  <input type='radio' name='web_DHT11_en' value='0' ";
  content += (DHT_img_flag == 0) ? "checked" : "";
  content += "> DIS <input type='radio' name='web_DHT11_en' value='1' ";
  content += (DHT_img_flag == 1) ? "checked" : "";
  content += "> EN<br>";
#endif
  content += "<br>LCD Rotation<br>\
                    <input type='radio' name='web_set_rotation' value='0'";
  content += (LCD_Rotation == 0) ? "checked" : "";
  content += "> USB Down<br>\
                    <input type='radio' name='web_set_rotation' value='1'";
  content += (LCD_Rotation == 1) ? "checked" : "";
  content += "> USB Right<br>\
                    <input type='radio' name='web_set_rotation' value='2'";
  content += (LCD_Rotation == 2) ? "checked" : "";
  content += "> USB Up<br>\
                    <input type='radio' name='web_set_rotation' value='3'";
  content += (LCD_Rotation == 3) ? "checked" : "";
  content += "> USB Left<br>";
  content += "<br><div><input type='submit' name='Save' value='Save'></form></div>" + msg + "<br>";
  content += "By WCY<br>";
  content += "<script> function clearinput(){document.getElementById('cityid').value='';}</script>"; 
  content += "</body>";
  content += "</html>";

  server.send(200, "text/html", content);
}

// no need authentication
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

String mdnsName;
// Web服务初始化
void Web_Sever_Init()
{
  uint32_t counttime = 0; // 记录创建mDNS的时间
  String chipid;
  chipid = WiFi.macAddress(); // The chip ID is essentially its MAC address(length: 6 bytes).
  mdnsName = "SD" + chipid.substring(12, 14);

  Serial.println("mDNS responder building...");
  counttime = millis();
  while (!MDNS.begin(mdnsName))
  {
    if (millis() - counttime > 30000)
      ESP.restart(); // 判断超过30秒钟就重启设备
  }

  Serial.println("mDNS responder started");
  // 输出连接wifi后的IP地址
  //  Serial.print("本地IP： ");
  //  Serial.println(WiFi.localIP());

  server.on("/", handleconfig);
  server.onNotFound(handleNotFound);

  // 开启TCP服务
  server.begin();
  Serial.println("HTTP服务器已开启");

  Serial.println("连接: http://" + mdnsName + ".local");
  Serial.print("本地IP： ");
  Serial.println(WiFi.localIP());
  // 将服务器添加到mDNS
  MDNS.addService("http", "tcp", 80);
}
// Web网页设置函数
void Web_Sever()
{
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Error:WiFi is not Connected.");
    return;    
  }
  // MDNS.update();
  server.handleClient();
}
// web服务打开后LCD显示登陆网址及IP
void Web_sever_Win()
{
  IPAddress IP_adr = WiFi.localIP();

  clk.setColorDepth(8);
  clk.loadFont("msyhbd20", LittleFS);
  clk.createSprite(220, 100);                         // 创建窗口
  clk.fillSprite(0x0000);                             // 填充率
  clk.drawRoundRect(0, 0, 200, 16, 8, 0xFFFF);        // 空心圆角矩形
  clk.fillRoundRect(3, 3, loadNum, 10, 5, TFT_GREEN); // 实心圆角矩形
  clk.setTextDatum(CC_DATUM);                         // 设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("WEB服务器已开启", 100, 40, 2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawRightString("可用网页进行配置", 180, 60, 2);
  clk.pushSprite(10, 120); // 窗口位置
  clk.deleteSprite();

  clk.createSprite(240, 20);
  clk.fillSprite(0x0000);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawCentreString("http://" + mdnsName + ".local", 120, 0, 0);
  clk.pushSprite(10, 100);
  clk.deleteSprite();

  clk.createSprite(240, 20);
  clk.fillSprite(0x0000);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_ORANGE, 0x0000);
  clk.drawCentreString("IP:" + IP_adr.toString(), 120, 0, 2);
  clk.pushSprite(0, 70);
  clk.deleteSprite();

  clk.unloadFont();
}
// 读取保存城市代码
void saveCityCodetoEEP(int *citycode)
{
  for (int cnum = 0; cnum < 5; cnum++)
  {
    EEPROM.write(CC_addr + cnum, *citycode % 100); // 城市地址写入城市代码
    EEPROM.commit();                               // 保存更改的数据
    *citycode = *citycode / 100;
    delay(5);
  }
}
void readCityCodefromEEP(int *citycode)
{
  for (int cnum = 5; cnum > 0; cnum--)
  {
    *citycode = *citycode * 100;
    *citycode += EEPROM.read(CC_addr + cnum - 1);
    delay(5);
  }
}
#endif