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
 *              MOSI  GPIO13
 *              RES   GPIO2
 *              DC    GPIO0
 *              LCDBL GPIO22
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
//#include <WiFi.h>
// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>
#include <HTTPClient.h>
// #include <ESP8266WebServer.h>
// #include <WebServer.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <EEPROM.h>
#include "qr.h"
#include "number.h"
#include "weathernum.h"
#include <NTPClient.h>
//#include <time.h>
//#include <sys/time.h>
#include <ESP32Time.h>

// Font files are stored in Flash FS
#include <FS.h>
#include <LittleFS.h>
#define FlashFS LittleFS

#define Version "SDD V1.3.4"
#define MaxScroll 50 // 定义最大滚动显示条数
/* *****************************************************************
 *  配置使能位
 * *****************************************************************/
// WEB配网使能标志位----WEB配网打开后会默认关闭smartconfig功能
#define WM_EN 1
// 设定DHT11温湿度传感器使能标志
#define DHT_EN 1
// 设置太空人图片是否使用
#define imgAst_EN 1

#if WM_EN
#include <WiFiManager.h>
// WiFiManager 参数
WiFiManager wm; // global wm instance
#endif

// 设定DHT11温湿度传感器引脚
#if DHT_EN
#include "DHT.h"
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#endif

/* ****************************************************************
 *  蓝牙相关变量函数
 *  为了学习，把经典蓝牙串口模式换成BLE模式。
 *  的确，如果是简单的串口应用，还是BluetoothSerial方便。
 *****************************************************************/
// 增加蓝牙串口功能 2023-6-4  增加BLE低能耗蓝牙功能 2023-6-10
// #include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// BluetoothSerial SerialBT;
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic; // 创建一个BLE特性pCharacteristic
BLEAdvertising *pAdvertising;
bool deviceConnected = false;    // 连接否标志位
bool oldDeviceConnected = false; // 上次连接标致
uint8_t txValue = 0;             // TX的值
long lastMsg = 0;                // 存放时间的变量
String rxload = "";              // RX的预置值

// See the following for generating UUIDs: https://www.uuidgenerator.net/
// a63016ab-fa9a-455f-b01b-7973c0f13055
// 19aef88e-7edf-4b82-b5b2-dc921ec39220
// d64b6849-4390-42a4-b1cd-6eff89803a21
#define SERVICE_UUID "a63016ab-fa9a-455f-b01b-7973c0f13055" // UART service UUID
#define CHARACTERISTIC_UUID_RX "a63016ab-fa9a-455f-b01b-7973c0f13055"
#define CHARACTERISTIC_UUID_TX "a63016ab-fa9a-455f-b01b-7973c0f13055"


/* *****************************************************************
 *  字库、图片库
 * *****************************************************************/
#include "font/ZdyLwFont_20.h"
#include "font/youyuan16.h"
#include "font/diansong20.h"
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
int AprevTime = 0; // 太空人更新时间记录
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

// 天气更新时间  X 分钟
int updateweater_time = 10;

//----------------------------------------------------

// LCD屏幕相关设置
TFT_eSPI tft = TFT_eSPI(); // 引脚请自行配置tft_espi库中的 User_Setup.h文件
TFT_eSprite clk = TFT_eSprite(&tft);

#define LCD_BL_PIN 22 // LCD背光引脚
uint16_t bgColor = 0x0000;

// 屏幕亮度调节
int pwm_channel0 = 0;
int pwm_freq = 5000;
// int pwm_value = 10;    //0-255

// 其余状态标志位
int LCD_Rotation = 0;        // LCD屏幕方向
int LCD_BL_PWM = 150;         // 屏幕亮度0-255，默认50
uint8_t Wifi_en = 1;         // wifi状态标志位  1：打开    0：关闭
uint8_t UpdateWeater_en = 0; // 更新时间标志位
int prevTime = 0;            // 滚动显示更新标志位
int DHT_img_flag = 0;        // DHT传感器使用标志位

time_t prevDisplay = 0;       // 显示时间显示记录
unsigned long weaterTime = 0; // 天气更新时间记录

/*** Component objects ***/
Number dig;
WeatherNum wrat;

uint32_t targetTime = 0;
String cityCode = "101281001"; // 天气城市代码 湛江：101281001长沙:101250101株洲:101250301衡阳:101250401
int tempnum = 0;               // 温度百分比
int huminum = 0;               // 湿度百分比
int tempcol = 0xffff;          // 温度显示颜色
int humicol = 0xffff;          // 湿度显示颜色

// EEPROM参数存储地址位
int BL_addr = 1;    // 被写入数据的EEPROM地址编号  1亮度
int Ro_addr = 2;    // 被写入数据的EEPROM地址编号  2 旋转方向
int DHT_addr = 3;   // 被写入数据的EEPROM地址编号  3 DHT使能标志位
int CC_addr = 10;   // 被写入数据的EEPROM地址编号  10城市
int wifi_addr = 30; // 被写入数据的EEPROM地址编号  20wifi-ssid-psw

// wifi连接UDP设置参数
WiFiUDP Udp;
WiFiClient wificlient;
unsigned int localPort = 8321;
float duty = 0;

// NTP服务器参数
//  static const char ntpServerName[] = "ntp6.aliyun.com";
const int timeZone = 8; // 东八区

WiFiUDP ntpUDP;
// IPAddress NtpIP = IPAddress(210,72,145,44);  //国家授时中心
NTPClient timeClient(Udp, "ntp.ntsc.ac.cn", 60 * 60 * timeZone, 60000);
ESP32Time rtc;   //用来管理系统时间


hw_timer_t *timer = NULL; // 声明一个定时器用来取NTP
long updateTime = 0;

//进度条
byte loadNum = 6;



/* *********************************************************/
/*  ***************函数定义**********************************/
/* *********************************************************/
void getCityCode();
void getCityWeater();
void saveParamCallback();
void scrollBanner();
void scrollDate();
void imgAnim();
void weaterData(String *cityDZ, String *dataSK, String *dataFC);
void getCityWeater();
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
void LCD_reflash(int en);
void savewificonfig();
void readwificonfig();
void deletewificonfig();
int StrSplit(String str, String fen, String *result);
//void MansetTime(int sc, int mn, int hr, int dy, int mt, int yr);
void setupBLE(String BLEName);
void IRAM_ATTR onTimer();
void ledcAnalogWrite(uint8_t channel, uint32_t value);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void Web_win();
void Webconfig();
void loading(byte delayTime); // 绘制进度条
void IndoorTem();
void Serial_set();
void sleepTimeLoop(uint8_t Maxlight, uint8_t Minlight);
void BluetoothProc();


/* *********************************************************/
/*  ********************************************************/
/* *********************************************************/


void setup()
{

  Serial.begin(115200);
  delay(1000); // 花一些时间打开串行监视器
  EEPROM.begin(1024);

  // 增加蓝牙串口功能
  // SerialBT.begin("ESP32_weather ");  //Bluetooth device name
  setupBLE("ESP32BLE_Weather"); // 设置蓝牙名称
  Serial.println("The device started, now you can pair it with bluetooth!");
  // WiFi.forceSleepWake();
  // wm.resetSettings();    //在初始化中使wifi重置，需重新配置WiFi

  // 屏幕亮度控制初始化
  //  initialize digital pin LED_BUILTIN as an output.
  ledcSetup(pwm_channel0, pwm_freq, 13);
  ledcAttachPin(LCD_BL_PIN, pwm_channel0);

  // 设置一个定时器网络对时
  timer = timerBegin(0, 80, true);             // 初始化定时器指针
  timerAttachInterrupt(timer, &onTimer, true); // 绑定定时器
  timerAlarmWrite(timer, 1000000, true);       // 配置报警计数器保护值（就是设置时间）单位uS  设置两小时
  timerAlarmEnable(timer);                     // 启用定时器 2 * 60 * 60 *

#if DHT_EN
  dht.begin();
  // 从eeprom读取DHT传感器使能标志
  DHT_img_flag = EEPROM.read(DHT_addr);
#endif
  // 从eeprom读取背光亮度设置
  if (EEPROM.read(BL_addr) > 0 && EEPROM.read(BL_addr) < 255)
    LCD_BL_PWM = EEPROM.read(BL_addr);
  // 从eeprom读取屏幕方向设置
  if (EEPROM.read(Ro_addr) >= 0 && EEPROM.read(Ro_addr) <= 3)
    LCD_Rotation = EEPROM.read(Ro_addr);

  ledcAnalogWrite(pwm_channel0, LCD_BL_PWM);

  tft.begin();          /* TFT init */
  tft.invertDisplay(1); // 反转所有显示颜色：1反转，0正常
  tft.setRotation(LCD_Rotation);
  tft.fillScreen(0x0000);
  tft.setTextColor(TFT_BLACK, bgColor);

  // 字库放在FS里面，查询一下是否正常
  //--------------------------------------------------------------------------
  if (!LittleFS.begin())
  {
    Serial.println("Flash FS initialisation failed!");
  }
  else
  {
    Serial.println("Flash FS available!");
  }

  // bool font_missing = false;
  // if (LittleFS.exists("/Auradiansong20.vlw") == false)
  //   font_missing = true;
  // if (LittleFS.exists("/msyh20.vlw") == false)
  //   font_missing = true;

  // if (font_missing)
  // {
  //   Serial.println("\nFont missing in Flash FS, did you upload it?");
  //   // while(1) yield();
  // }
  // else
  //   Serial.println("\nFonts found OK.");

  //-------------------------------------------------------------------------

  targetTime = millis() + 1000;
  readwificonfig(); // 读取存储的wifi信息
  Serial.print("正在连接WIFI ");
  Serial.println(wificonf.stassid);

  WiFi.begin(wificonf.stassid, wificonf.stapsw);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(true);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  while (WiFi.status() != WL_CONNECTED)
  {
    loading(30);

    if (loadNum >= 255)
    {
// 使能web配网后自动将smartconfig配网失效
#if WM_EN
      Web_win();
      Webconfig();
#endif

#if !WM_EN
      SmartConfig();
#endif
      break;
    }
  }

  delay(10);
  while (loadNum < 255) // 让动画走完
  {
    loading(1);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("SSID:");
    Serial.println(WiFi.SSID().c_str());
    Serial.print("PSW:");
    Serial.println("************");
    // Serial.println(WiFi.psk().c_str());
    strcpy(wificonf.stassid, WiFi.SSID().c_str()); // 名称复制
    strcpy(wificonf.stapsw, WiFi.psk().c_str());   // 密码复制
    savewificonfig();
    readwificonfig();
  }

  Serial.print("本地IP： ");
  Serial.println(WiFi.localIP());
  Serial.println("启动UDP");
  Udp.begin(localPort);
  Serial.println("等待同步...");

  // 每 60 * 60 秒同步时间一次
  timeClient.begin();
  setSyncProvider(getNtpTime);
  setSyncInterval(60 * 60);   //每1小时同步一次时间
  timeClient.update();

  int Maxtry = 0;
  while (rtc.getYear() == 1970)
  {
    if (Maxtry > 10){
      Serial.println("获取NTP时间失败，手动设置一个时间：2023-6-18-1-1-1"); //如果同步失败，手动设置一个系统时间。
      rtc.setTime(01, 01, 01, 18, 6, 2023);  // 17th Jan 2021 15:24:30
      break;      
    }
    delay(1000);
    timeClient.update();
    rtc.setTime(getNtpTime()); 
    Serial.println("正在同步。。。" + (String)Maxtry);
    Maxtry++;
  }

  //开始相关显示
  tft.fillScreen(TFT_BLACK); // 清屏
//显示温湿度图标和方框
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  TJpgDec.drawJpg(31, 183, temperature, sizeof(temperature)); // 温度图标
  TJpgDec.drawJpg(50, 200, humidity, sizeof(humidity));       // 湿度图标
  tft.drawRoundRect(170, 112, 66, 48, 5, TFT_YELLOW);         // 室内温湿度框

  //获取城市代码
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

  getCityWeater(); // 取天气情况
  getNongli();     // 农历信息

#if DHT_EN
  if (DHT_img_flag != 0)
    IndoorTem();
#endif

  if (WiFi.status() == WL_CONNECTED)
  {

    if (WiFi.getSleep())
    {
      Serial.println("WIFI休眠成功......");
      Wifi_en = 0;
    }
    else
      Serial.println("WIFI休眠失败......");
  }
}

void loop()
{
  LCD_reflash(0);
  Serial_set();
  sleepTimeLoop(LCD_BL_PWM, 120); // 定时开关显示屏背光 参数是打开后最大亮度
  BluetoothProc();           // BTE蓝牙处理
}

/* ***************************************************************************
  *  以下各类函数
  *
  * **************************************************************************/
void IRAM_ATTR onTimer()
{ // 定时器中断函数
  updateTime++;
}

// 服务器回调
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.print("BLE 连接上。");
    Serial.println();
  };
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.print("BLE 断开了。");
    Serial.println();
  }
};

// 特性回调
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0)
    {
      rxload = "";
      for (int i = 0; i < rxValue.length(); i++)
      {
        rxload += (char)rxValue[i];
        Serial.print(rxValue[i]);
      }
      Serial.println("");
    }
  }
};

void setupBLE(String BLEName)
{
  const char *ble_name = BLEName.c_str(); // 将传入的BLE的名字转换为指针
  BLEDevice::init(ble_name);              // 初始化一个蓝牙设备

  pServer = BLEDevice::createServer();            // 创建一个蓝牙服务器
  pServer->setCallbacks(new MyServerCallbacks()); // 服务器回调函数设置为MyServerCallbacks

  BLEService *pService = pServer->createService(SERVICE_UUID); // 创建一个BLE服务

  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  // 创建一个(读)特征值 类型是通知
  pCharacteristic->addDescriptor(new BLE2902());
  // 为特征添加一个描述

  // BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  // 创建一个(写)特征 类型是写入
  pCharacteristic->setCallbacks(new MyCallbacks());
  // 为特征添加一个回调

  pService->start(); // 开启服务

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  // pServer->getAdvertising()->start(); //服务器开始广播
  Serial.println("Waiting a client connection to notify...");
}

// 这里回传的信息字节被限制在20+3字节，如要增加，
// 需要在手机蓝牙调试APP里面设置MTU限制数到合适位置。
// 建议设置在128，可以宽松收完菜单提示信息。
int SentBT(String data)
{

  if (deviceConnected)
  {
    // char BLEbuf[32];
    const char *ch;

    ch = data.c_str();

    // memset(BLEbuf, 0, 32);
    //    memcpy(BLEbuf, num, sizeof(num));
    // emcpy(*ch, ch, 32);
    pCharacteristic->setValue(ch);
    pCharacteristic->notify(); // Send the value to the app!
    Serial.print("*** Sent Value: ");
    Serial.print(ch);
    Serial.println(" ***");
    return 1;
  }
  else
  {
    Serial.print("BT Sent fail!!!");
    return 0;
  }
}

/* ****************************************************************
 *  蓝牙相关变量函数结束
 ****************************************************************/



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
  // EEPROM.commit();
  // ssid = wificonf.stassid;
  // pass = wificonf.stapsw;
  Serial.printf("Read WiFi Config.....\r\n");
  Serial.printf("SSID:%s\r\n", wificonf.stassid);
  Serial.print("************");
  //Serial.printf("PSW:%s\r\n", wificonf.stapsw);
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
  clk.fillRoundRect(3, 3, loadNum, 10, 5, 0xFFFF); // 实心圆角矩形
  clk.setTextDatum(CC_DATUM);                      // 设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("Connecting to WiFi......", 100, 40, 2);
  clk.setTextColor(TFT_WHITE, 0x0000);
  clk.drawRightString(Version, 180, 60, 2);
  clk.pushSprite(20, 120); // 窗口位置

  // clk.setTextDatum(CC_DATUM);
  // clk.setTextColor(TFT_WHITE, 0x0000);
  // clk.pushSprite(130,180);

  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}

// 湿度图标显示函数
void humidityWin()
{
  clk.setColorDepth(8);

  huminum = huminum * (46 - 5) / 100;
  clk.createSprite(46, 6);                         // 创建窗口
  clk.fillSprite(0x0000);                          // 填充率
  clk.drawRoundRect(0, 0, 46, 6, 3, 0xFFFF);       // 空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1, 1, huminum, 4, 2, humicol); // 实心圆角矩形
  clk.pushSprite(70, 222);                         // 窗口位置
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
// 外接DHT11传感器，显示数据
void IndoorTem()
{
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  // String s = "内温";
  // /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);

  // //位置
  // clk.createSprite(58, 30);
  // clk.fillSprite(bgColor);
  // clk.setTextDatum(CC_DATUM);
  // clk.setTextColor(TFT_WHITE, bgColor);
  // clk.drawString(s, 29, 16);
  // clk.pushSprite(172, 120);//150
  // clk.deleteSprite();

  // 温度
  clk.createSprite(60, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_CYAN, bgColor);
  clk.drawFloat(t, 1, 20, 13);
  //  clk.drawString(sk["temp"].as<String>()+"℃",28,13);
  clk.drawString("℃", 50, 13);
  clk.pushSprite(173, 112); // 184
  clk.deleteSprite();

  // 湿度
  clk.createSprite(60, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREENYELLOW, bgColor);
  //  clk.drawString(sk["SD"].as<String>(),28,13);
  clk.drawFloat(h, 1, 20, 13);
  clk.drawString("%", 50, 13);
  // clk.drawString("100%",28,13);
  clk.pushSprite(173, 136); // 214
  clk.deleteSprite();

  // void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color)
  // tft.drawRoundRect(170, 112, 66, 48, 5, TFT_YELLOW);
}
#endif

#if !WM_EN
// 微信配网函数
void SmartConfig(void)
{
  WiFi.mode(WIFI_STA); // 设置STA模式
  // tft.pushImage(0, 0, 240, 240, qr);
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
  if (Serial.available() > 0 || rxload.length() > 0)
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
    // 蓝牙串口
    if (deviceConnected && rxload.length() > 0)
    {
      // incomingByte = rxload;
      for (int i = 0; i < rxload.length(); i++)
      {
        if (iscntrl(rxload.charAt(i)))
        { // 去掉控制符
          i++;
        }
        else
        {
          incomingByte += rxload.charAt(i);
        }
      }
      rxload = "";
      isBlueT = 1;
    }
    // Serial.print(rxload.length());
    // Serial.println();
    // Serial.print(incomingByte);
    // Serial.println();

    // if (SerialBT.available() > 0) {
    //   while (SerialBT.available() > 0)  //监测串口缓存，当有数据输入时，循环赋值给incomingByte
    //   {
    //     if (iscntrl(SerialBT.peek()))
    //       SerialBT.read();
    //     else
    //       incomingByte += char(SerialBT.read());  //读取单个字符值，转换为字符，并按顺序一个个赋值给incomingByte
    //     delay(2);                                 //不能省略，因为读取缓冲区数据需要时间
    //   }
    //   isBlueT = 1;
    // }

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
        isBlueT ? SentBT("亮度调整为：") : Serial.printf("亮度调整为：");
        // Serial.printf("亮度调整为：");
        // analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
        ledcAnalogWrite(pwm_channel0, LCD_BL_PWM);
        // Serial.println(LCD_BL_PWM);
        isBlueT ? SentBT("LCD_BL_PWM") : Serial.println("LCD_BL_PWM");
        // Serial.println("");
        isBlueT ? SentBT("") : Serial.println("");
      }
      else
        isBlueT ? SentBT("亮度调整错误，请输入0-255") : Serial.println("亮度调整错误，请输入0-255");
      // Serial.println("亮度调整错误，请输入0-255");
    }
    if (SMOD == "0x02") // 设置2地址设置
    {
      unsigned int CityCODE = 0;
      unsigned int CityC = atoi(incomingByte.c_str()); // int n = atoi(xxx.c_str());//String转int
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
          isBlueT ? SentBT("城市代码调整为：自动") : Serial.println("城市代码调整为：自动");
          // Serial.println("城市代码调整为：自动");
          getCityCode(); // 获取城市代码
        }
        else
        {
          isBlueT ? SentBT("城市代码调整为：") : Serial.printf("城市代码调整为：");
          // Serial.printf("城市代码调整为：");
          isBlueT ? SentBT("cityCode") : Serial.println("cityCode");
          // Serial.println(cityCode);
        }
        // Serial.println("");
        isBlueT ? SentBT("") : Serial.println("");
        getCityWeater(); // 更新城市天气
        SMOD = "";
      }
      else
        isBlueT ? SentBT("城市调整错误，请输入9位城市代码，自动获取请输入0") : Serial.println("城市调整错误，请输入9位城市代码，自动获取请输入0");
      // Serial.println("城市调整错误，请输入9位城市代码，自动获取请输入0");
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
        LCD_reflash(1); // 屏幕刷新程序
        UpdateWeater_en = 1;
        TJpgDec.drawJpg(15, 183, temperature, sizeof(temperature)); // 温度图标
        TJpgDec.drawJpg(15, 213, humidity, sizeof(humidity));       // 湿度图标
        // getCityWeater();
        // digitalClockDisplay(1);
        // scrollBanner();
        // #if DHT_EN
        //   if(DHT_img_flag == 1)
        //   IndoorTem();
        // #endif
        // #if imgAst_EN
        //   if(DHT_img_flag == 0)
        //   imgAnim();
        // #endif

        // Serial.print("屏幕方向设置为：");
        isBlueT ? SentBT("屏幕方向设置为：") : Serial.printf("亮度屏幕方向设置为：调整为：");
        // Serial.println(RoSet);
        isBlueT ? SentBT("RoSet") : Serial.println("RoSet");
      }
      else
      {
        // Serial.println("屏幕方向值错误，请输入0-3内的值");
        isBlueT ? SentBT("屏幕方向值错误，请输入0-3内的值") : Serial.println("屏幕方向值错误，请输入0-3内的值");
      }
    }
    if (SMOD == "0x04") // 设置天气更新时间
    {
      int wtup = atoi(incomingByte.c_str()); // int n = atoi(xxx.c_str());//String转int
      if (wtup >= 1 && wtup <= 60)
      {
        updateweater_time = wtup;
        SMOD = "";
        // Serial.printf("天气更新时间更改为：");
        isBlueT ? SentBT("天气更新时间更改为：") : Serial.printf("天气更新时间更改为：");
        // Serial.print(updateweater_time);
        isBlueT ? SentBT("updateweater_time") : Serial.print("updateweater_time");
        // Serial.println("分钟");
        isBlueT ? SentBT("分钟") : Serial.println("分钟");
      }
      else
        // Serial.println("更新时间太长，请重新设置（1-60）");
        isBlueT ? SentBT("更新时间太长，请重新设置（1-60）") : Serial.println("更新时间太长，请重新设置（1-60）");
    }
    else
    {
      SMOD = incomingByte;
      delay(2);
      if (SMOD == "0x01")
        // Serial.println("请输入亮度值，范围0-255");
        isBlueT ? SentBT("请输入亮度值，范围0-255") : Serial.println("请输入亮度值，范围0-255");
      else if (SMOD == "0x02")
        // Serial.println("请输入9位城市代码，自动获取请输入0");
        isBlueT ? SentBT("请输入9位城市代码，自动获取请输入0") : Serial.println("请输入9位城市代码，自动获取请输入0");
      else if (SMOD == "0x03")
      {
        // Serial.println("请输入屏幕方向值，");
        isBlueT ? SentBT("请输入屏幕方向值，") : Serial.println("请输入屏幕方向值，");
        // Serial.println("0-USB接口朝下");
        isBlueT ? SentBT("0-USB接口朝下") : Serial.println("0-USB接口朝下");
        // Serial.println("1-USB接口朝右");
        isBlueT ? SentBT("1-USB接口朝右") : Serial.println("1-USB接口朝右");
        // Serial.println("2-USB接口朝上");
        isBlueT ? SentBT("2-USB接口朝上") : Serial.println("2-USB接口朝上");
        // Serial.println("3-USB接口朝左");
        isBlueT ? SentBT("3-USB接口朝") : Serial.println("3-USB接口朝");
      }
      else if (SMOD == "0x04")
      {
        // Serial.print("当前天气更新时间：");
        isBlueT ? SentBT("当前天气更新时间：") : Serial.printf("当前天气更新时间：");
        // Serial.print(updateweater_time);
        isBlueT ? SentBT("updateweater_time") : Serial.printf("updateweater_time");
        // Serial.println("分钟");
        isBlueT ? SentBT("分钟") : Serial.println("分钟");
        // Serial.println("请输入天气更新时间（1-60）分钟");
        isBlueT ? SentBT("请输入天气更新时间（1-60）分钟") : Serial.println("请输入天气更新时间（1-60）分钟");
      }
      else if (SMOD == "0x05")
      {
        // Serial.println("重置WiFi设置中......");
        isBlueT ? SentBT("重置WiFi设置中......") : Serial.println("重置WiFi设置中......");
        delay(10);
        wm.resetSettings();
        deletewificonfig();
        delay(10);
        // Serial.println("重置WiFi成功");
        isBlueT ? SentBT("重置WiFi成功") : Serial.println("重置WiFi成功");
        SMOD = "";
        ESP.restart();
      }
      else
      {
        // Serial.println("");
        isBlueT ? SentBT("") : Serial.println("");
        // Serial.println("请输入需要修改的代码：");
        isBlueT ? SentBT("请输入需要修改的代码：") : Serial.println("请输入需要修改的代码：");
        // Serial.println("亮度设置输入        0x01");
        isBlueT ? SentBT("亮度设置输入        0x01") : Serial.println("亮度设置输入        0x01");
        // Serial.println("地址设置输入        0x02");
        isBlueT ? SentBT("地址设置输入        0x02") : Serial.println("地址设置输入        0x02");
        // Serial.println("屏幕方向设置输入    0x03");
        isBlueT ? SentBT("屏幕方向设置输入    0x03") : Serial.println("屏幕方向设置输入    0x03");
        // Serial.println("更改天气更新时间    0x04");
        isBlueT ? SentBT("更改天气更新时间    0x04") : Serial.println("更改天气更新时间    0x04");
        // Serial.println("重置WiFi(会重启)    0x05");
        isBlueT ? SentBT("重置WiFi(会重启)    0x05") : Serial.println("重置WiFi(会重启)    0x05");
        // Serial.println("");
        isBlueT ? SentBT("") : Serial.println("");
      }
    }
  }
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
  //int customFieldLength = 40;

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
  WiFiManagerParameter custom_bl("LCDBL", "LCD BackLight(1-100)", "10", 3);
#if DHT_EN
  WiFiManagerParameter custom_DHT11("DHT11EN", "Enable DHT11 sensor", "1", 3);
#endif
  WiFiManagerParameter custom_weatertime("WeaterUpdateTime", "Weather Update Time(Min)", "10", 3);
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
  chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  String StrSSID = "WeatherAP_" + String((unsigned long)(uint16_t)chipid, HEX);

  res = wm.autoConnect(StrSSID.c_str()); // anonymous ap
    //res = wm.autoConnect("AutoConnectAP" + String((uint32_t)ESP.getEfuseMac())); // anonymous ap
  //  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

  while (!res){
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
// Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
// Serial.println("PARAM CityCode = " + getParam("CityCode"));
// Serial.println("PARAM LCD BackLight = " + getParam("LCDBL"));
// Serial.println("PARAM WeaterUpdateTime = " + getParam("WeaterUpdateTime"));
// Serial.println("PARAM Rotation = " + getParam("set_rotation"));
// Serial.println("PARAM DHT11_en = " + getParam("DHT11_en"));

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
  // 屏幕亮度
  Serial.printf("亮度调整为：");
  // analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
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
#endif


bool haveNL = 0 ;//每天凌晨更新农历
void LCD_reflash(int en)
{
  if (now() != prevDisplay || en == 1)
  {
    prevDisplay = now();
    digitalClockDisplay(en);
    prevTime = 0;
  }

  // 一分钟更新一次室内温度
#if DHT_EN
  if (second() % 60 == 0 && prevTime == 0 || en == 1)
  {

    if (DHT_img_flag != 0)
      IndoorTem();
  }
#endif
  // 两秒钟更新一次
  if (second() % 2 == 0 && prevTime == 0 || en == 1)
  {
    scrollBanner();
    scrollDate();
  }

  imgAnim();

  if (millis() - weaterTime > (60000 * updateweater_time) || en == 1 || UpdateWeater_en != 0)
  { // 10分钟更新一次天气

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WIFI已连接");
      getCityWeater();
      if (UpdateWeater_en != 0)
        UpdateWeater_en = 0;
      weaterTime = millis();

      if (WiFi.getSleep())
      {
        Serial.println("WIFI休眠成功......");
        Wifi_en = 0;
      }
      else
        Serial.println("WIFI休眠失败......");
    }
  }

  //每天凌晨8分更新一下农历信息
  // Serial.println("hours:" + String(rtc.getHour()));
  // Serial.println("getMinutes:" + String(rtc.getMinute()));

  if (rtc.getHour(true) == 0 && rtc.getMinute() == 8)
  {
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

    // 定时器计数 秒  
  if (updateTime > 5)
  {
    updateTime = 0;
  }
}

// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode()
{
  //String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());
  String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(rtc.getEpoch());
  // 创建 HTTPClient 对象
  HTTPClient httpClient;

  Serial.println(URL);

  // 配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(wificlient, URL);

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);

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

      // getCityWeater();
    }
    else
    {
      Serial.println("获取城市代码失败");
    }
  }
  else
  {
    Serial.println("请求城市代码错误：");
    Serial.println(httpCode);
  }

  // 关闭ESP8266与服务器连接
  httpClient.end();
}

// 获取城市天气
void getCityWeater()
{
  // String URL = "http://d1.weather.com.cn/dingzhi/" + cityCode + ".html?_="+String(now());//新getCityCodedata
  //String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(now()); // 原来
  String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(rtc.getEpoch()); 

  Serial.println(URL);
  // 创建 HTTPClient 对象
  HTTPClient httpClient;

  httpClient.begin(URL);

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");
  // Serial.println(URL);

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {

    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");

    String jsonCityDZ = str.substring(indexStart + 13, indexEnd);
    // Serial.println(jsonCityDZ);

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart + 8, indexEnd);
    // Serial.println(jsonDataSK);

    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart + 5, indexEnd);
    // Serial.println(jsonFC);

    weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC);
    Serial.println("获取成功");
  }
  else
  {
    Serial.print("请求城市天气错误：");
    Serial.println(httpCode);
  }

  // 关闭ESP8266与服务器连接
  httpClient.end();
}

//设计一个结构体，实现不同意思字显示不同颜色
#define cRED    0
#define cGREEN  1
#define cWHITE  2

struct Display
{
   String  title = "";
   int     color = 0;
};

Display scrollNongLi[MaxScroll];

//String scrollNongLi[MaxScroll] = {""};
// 获取农历信息
void getNongli()
{
  DynamicJsonDocument doc(1024);
  String id = "ukyvqbejoesbxpqk";
  String secret = "dGltRE9zenJCZWFLUWZOVSs3Rm42dz09";
  // String Y = String(year());
  // String M = month() < 10 ? "0" + String(month()) : String(month());
  // String D = day() < 10 ? "0" + String(day()) : String(day());

   String Y = String(rtc.getYear());
   String M = rtc.getMonth() + 1 < 10 ? "0" + String(rtc.getMonth() + 1) : String(rtc.getMonth() + 1);
   String D = rtc.getDay() < 10 ? "0" + String(rtc.getDay()) : String(rtc.getDay());

  memset(scrollNongLi, '\0', sizeof(scrollNongLi));
  scrollNongLi[0].title = monthDay() + " " + week();
  scrollNongLi[0].color = cWHITE;

  // https://www.mxnzp.com/api/holiday/single/20181121?ignoreHoliday=false&app_id=不再提供请自主申请&app_secret=不再提供请自主申请
  String URL = "https://www.mxnzp.com/api/holiday/single/" + Y + M + D + "?ignoreHoliday=false&app_id=" + id + "&app_secret=" + secret;

  Serial.println(URL);
  // 创建 HTTPClient 对象
  HTTPClient httpClient;

  httpClient.begin(URL);

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "https://www.mxnzp.com/");

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取农历数据");
  // Serial.println(URL);

  Serial.println("httpcode:" + httpCode);

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {

    String response = httpClient.getString();

    // 反序列化JSON
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonObject root = doc.as<JsonObject>();
    JsonObject data = root["data"];

    //const char *yearTips = data["yearTips"];           // 天干地支纪年法描述 例如：戊戌
    //const char *typeDes = data["typeDes"];             // 类型描述 比如 国庆,休息日,工作日 如果ignoreHoliday参数为true，这个字段不返回
    //const char *chineseZodiac = data["chineseZodiac"]; // 属相
    //const char *solarTerms = data["solarTerms"];       // 节气
    //const char *lunarCalendar = data["lunarCalendar"]; // 农历
    const char *suit = data["suit"];                   // 今日宜
    //const char *weekOfYear = data["weekOfYear"];       // 第几周
    const char *avoid = data["avoid"];                 // 今日忌

    scrollNongLi[1].title = data["yearTips"].as<String>() + "年 " + data["lunarCalendar"].as<String>();
    scrollNongLi[1].color = cWHITE;
    scrollNongLi[2].title = data["chineseZodiac"].as<String>() + "年" + data["weekOfYear"].as<String>() + "周" + " " + data["typeDes"].as<String>();
    scrollNongLi[2].color = cWHITE;

    // 宜忌有时描述太长，分行显示
    String temp = "";
    int TotalWord = MaxScroll - 3;
    String rword[TotalWord] = {""};
    int tal = 0;
    int needle = 3;

    memset(rword, '\0', sizeof(rword));

    temp = suit;
    if (temp.length() > 0)
    {
      tal = StrSplit(temp, ".", rword);
      Serial.println(tal);
      if (tal < MaxScroll)
      {
        for (int i = 0; i * 2 <= tal + 1; i += 2)
        {
          scrollNongLi[needle].title = "宜:" + rword[i] + " " + rword[i + 1];
          scrollNongLi[needle].color = cGREEN;
          Serial.println(scrollNongLi[needle].title);
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
    else{
      scrollNongLi[needle].title = "宜:";
      scrollNongLi[needle].color = cWHITE;
    }

    memset(rword, '\0', sizeof(rword));
    temp = avoid;
    if (temp.length() > 0)
    {
      tal = StrSplit(temp, ".", rword);
      Serial.println(tal);
      if (tal < MaxScroll)
      {
        for (int i = 0; i * 2 <= tal + 1; i += 2)
        {
          scrollNongLi[needle].title = "忌:" + rword[i] + " " + rword[i + 1];
          scrollNongLi[needle].color = cRED;

          Serial.println(scrollNongLi[needle].title);
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
    else{
      scrollNongLi[needle].title = "忌:";
      scrollNongLi[needle].color = cRED;
    }

    // scrollNongLi[3] = "宜:" + data["suit"].as<String>();
    // scrollNongLi[4] = "忌:" + data["avoid"].as<String>();

    // Serial.println(strlen(yearTips));
    // Serial.println(typeDes);
    // Serial.println(chineseZodiac);
    // Serial.println(solarTerms);
    // Serial.println(lunarCalendar);
    // Serial.println(strlen(suit));
    // Serial.println(weekOfYear);

    Serial.println("打印结束");

    // int indexStart = str.indexOf("weatherinfo\":");
    // int indexEnd = str.indexOf("};var alarmDZ");

    // String jsonCityDZ = str.substring(indexStart + 13, indexEnd);
    // //Serial.println(jsonCityDZ);

    // indexStart = str.indexOf("dataSK =");
    // indexEnd = str.indexOf(";var dataZS");
    // String jsonDataSK = str.substring(indexStart + 8, indexEnd);
    // //Serial.println(jsonDataSK);

    // indexStart = str.indexOf("\"f\":[");
    // indexEnd = str.indexOf(",{\"fa");
    // String jsonFC = str.substring(indexStart + 5, indexEnd);
    // Serial.println(jsonFC);

    // weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC);
    Serial.println("获取成功");
  }
  else
  {
    Serial.print("请求农历信息错误：");
    Serial.println(httpCode);
  }

  // 关闭ESP8266与服务器连接
  httpClient.end();
}

String scrollText[7];
// int scrollTextWidth = 0;
// 天气信息写到屏幕上
void weaterData(String *cityDZ, String *dataSK, String *dataFC)
{
  // 解析第一段JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  // TFT_eSprite clkb = TFT_eSprite(&tft);

  /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20); // ZdyLwFont_20

  // 温度
  clk.createSprite(58, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(sk["temp"].as<String>() + "℃", 28, 13);
  clk.pushSprite(110, 184);
  clk.deleteSprite();
  tempnum = sk["temp"].as<int>();
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
  clk.drawString(sk["SD"].as<String>(), 28, 13);
  // clk.drawString("100%",28,13);
  clk.pushSprite(110, 214);
  clk.deleteSprite();
  // String A = sk["SD"].as<String>();
  huminum = atoi((sk["SD"].as<String>()).substring(0, 2).c_str());

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
  clk.drawString(sk["cityname"].as<String>(), 44, 16);
  clk.pushSprite(26, 15); // 15,15
  clk.deleteSprite();

  // PM2.5空气指数
  uint16_t pm25BgColor = tft.color565(156, 202, 127); // 优
  String aqiTxt = "优";
  int pm25V = sk["aqi"];
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

  scrollText[0] = "实时天气 " + sk["weather"].as<String>();
  scrollText[1] = "空气质量 " + aqiTxt;
  scrollText[2] = "风向 " + sk["WD"].as<String>() + sk["WS"].as<String>();

  // scrollText[6] = atoi((sk["weathercode"].as<String>()).substring(1,3).c_str()) ;

  // 天气图标  170,15
  wrat.printfweather(160, 15, atoi((sk["weathercode"].as<String>()).substring(1, 3).c_str()));

  // 左上角滚动字幕
  // 解析第二段JSON
  deserializeJson(doc, *cityDZ);
  JsonObject dz = doc.as<JsonObject>();
  // Serial.println(sk["ws"].as<String>());
  // 横向滚动方式
  // String aa = "今日天气:" + dz["weather"].as<String>() + "，温度:最低" + dz["tempn"].as<String>() + "，最高" + dz["temp"].as<String>() + " 空气质量:" + aqiTxt + "，风向:" + dz["wd"].as<String>() + dz["ws"].as<String>();
  // scrollTextWidth = clk.textWidth(scrollText);
  // Serial.println(aa);
  scrollText[3] = "今日" + dz["weather"].as<String>();

  deserializeJson(doc, *dataFC);
  JsonObject fc = doc.as<JsonObject>();

  scrollText[4] = "最低温度" + fc["fd"].as<String>() + "℃";
  scrollText[5] = "最高温度" + fc["fc"].as<String>() + "℃";

  // Serial.println(scrollText[0]);

  clk.unloadFont();
}

int currentIndex = 0;
TFT_eSprite clkb = TFT_eSprite(&tft);

void scrollBanner()
{
  // if(millis() - prevTime > 2333) //3秒切换一次
  //   if(second()%2 ==0&& prevTime == 0)
  //   {
  if (scrollText[currentIndex])
  {
    clkb.setColorDepth(8);
    clkb.loadFont(ZdyLwFont_20);
    clkb.createSprite(150, 30);
    clkb.fillSprite(bgColor);
    clkb.setTextWrap(false);
    clkb.setTextDatum(CC_DATUM);
    clkb.setTextColor(TFT_WHITE, bgColor);
    clkb.drawString(scrollText[currentIndex], 74, 16);
    clkb.pushSprite(10, 45);

    clkb.deleteSprite();
    clkb.unloadFont();

    if (currentIndex >= 5)
      currentIndex = 0; // 回第一个
    else
      currentIndex += 1; // 准备切换到下一个
  }
  prevTime = 1;
  //  }
}

int CurrentDisDate = 0;
void scrollDate()
{
  if (scrollNongLi[CurrentDisDate].title)
  {
    /***日期****/
    clk.setColorDepth(8);

    // 新的装载字库的方法：
    // 直接先用processing生成xxx.vlw 格式的文件
    // 把xxx.vlw放在platfomio项目下创建的data目录
    // 把vlw文件上传到单片机的flash空间中
    // 然后在这里调用
    //clk.loadFont("msyh20", LittleFS);
    clk.loadFont("msyhbd20", LittleFS);
    // clk.loadFont(diansong20);

    // 星期
    clk.createSprite(162, 30);
    clk.fillSprite(bgColor);
    clk.setTextDatum(CC_DATUM);
    //clk.setTextColor(TFT_WHITE, bgColor);

    //Serial.println("currentD:" + String(CurrentDisDate) + ":" + scrollNongLi[CurrentDisDate] );

    if (scrollNongLi[CurrentDisDate].title != ""){

      switch (scrollNongLi[CurrentDisDate].color)
      {
      case cRED:
          clk.setTextColor(tft.color565(254, 67, 101), bgColor);
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
    }
    else
    {
      CurrentDisDate = 0;
    }

    clk.pushSprite(10, 150);
    clk.deleteSprite();
    clk.unloadFont();

    if (CurrentDisDate >= MaxScroll - 1)
      CurrentDisDate = 0; // 回第一个
    else
      CurrentDisDate += 1; // 准备切换到下一个
  }
  else{
      CurrentDisDate = 0; // 回第一个
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
  if (hour() != Hour_sign || reflash_en == 1) // 时钟刷新
  {
    dig.printfW3660(20 - 10, timey, hour() / 10);
    dig.printfW3660(60 - 10, timey, hour() % 10);
    Hour_sign = hour();
  }
  if (minute() != Minute_sign || reflash_en == 1) // 分钟刷新
  {
    dig.printfO3660(101 - 10, timey, minute() / 10);
    dig.printfO3660(141 - 10, timey, minute() % 10);
    Minute_sign = minute();
  }
  if (second() != Second_sign || reflash_en == 1) // 秒钟刷新
  {
    dig.printfW1830(182 - 10, timey, second() / 10);
    dig.printfW1830(202 - 10, timey, second() % 10);
    // dig.printfW1830(182, timey + 30, second() / 10);
    // dig.printfW1830(202, timey + 30, second() % 10);
    Second_sign = second();
  }

  if (reflash_en == 1)
    reflash_en = 0;
}

// 星期
String week()
{
  String wk[7] = {"日", "一", "二", "三", "四", "五", "六"};
  String s = "周" + wk[weekday() - 1];
  return s;
}

// 月日
String monthDay()
{
  String s = String(month());
  s = s + "月" + day() + "日";
  return s;
}

/*-------- NTP code ----------*/

time_t getNtpTime()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    timeClient.update();
    // return timeClient.getFormattedTime();
    Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));      // (String) returns time with specified format
    return timeClient.getEpochTime();
  }
  return 0;
}

// const int NTP_PACKET_SIZE = 48;      // NTP时间在消息的前48字节中
// byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming & outgoing packets

// time_t getNtpTime() {
//   IPAddress ntpServerIP;  // NTP server's ip address

//   while (Udp.parsePacket() > 0) {};  // discard any previously received packets
//   //Serial.println("Transmit NTP Request");
//   // get a random server from the pool
//   WiFi.hostByName(ntpServerName, ntpServerIP);
//   //Serial.print(ntpServerName);
//   //Serial.print(": ");
//   //Serial.println(ntpServerIP);
//   sendNTPpacket(ntpServerIP);
//   uint32_t beginWait = millis();
//   while (millis() - beginWait < 1500) {
//     int size = Udp.parsePacket();
//     if (size >= NTP_PACKET_SIZE) {
//       Serial.println("Receive NTP Response");
//       Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
//       unsigned long secsSince1900;
//       // convert four bytes starting at location 40 to a long integer
//       secsSince1900 = (unsigned long)packetBuffer[40] << 24;
//       secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
//       secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
//       secsSince1900 |= (unsigned long)packetBuffer[43];
//       //Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
//       return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
//     }
//   }
//   Serial.println("No NTP Response :-(");
//   return 0;  // 无法获取时间时返回0
// }

// // 向NTP服务器发送请求
// void sendNTPpacket(IPAddress &address) {
//   // set all bytes in the buffer to 0
//   memset(packetBuffer, 0, NTP_PACKET_SIZE);
//   // Initialize values needed to form NTP request
//   // (see URL above for details on the packets)
//   packetBuffer[0] = 0b11100011;  // LI, Version, Mode
//   packetBuffer[1] = 0;           // Stratum, or type of clock
//   packetBuffer[2] = 6;           // Polling Interval
//   packetBuffer[3] = 0xEC;        // Peer Clock Precision
//   // 8 bytes of zero for Root Delay & Root Dispersion
//   packetBuffer[12] = 49;
//   packetBuffer[13] = 0x4E;
//   packetBuffer[14] = 49;
//   packetBuffer[15] = 52;
//   // all NTP fields have been given values, now
//   // you can send a packet requesting a timestamp:
//   Udp.beginPacket(address, 123);  //NTP requests are to port 123
//   Udp.write(packetBuffer, NTP_PACKET_SIZE);
//   Udp.endPacket();
// }

/* **************************************************************************
/* *******************设置显示屏休眠时间***************************************
/* **************************************************************************/

int sethourK = 21; // 几点比如输入01表示1点，13表示13点
int setminK = 00;  // 几分开比如输入05表示5分，22表示22分

int sethourG = 8; // 几点关
int setminG = 30; // 几分关
bool isSleepMode = 0;
int bLight = LCD_BL_PWM;
int preTime;

/**
 * @brief 设置睡眠时间
 *
 * @param data
 */
void sleepTimeLoop(uint8_t Maxlight, uint8_t Minlight)
{
  int starttime = sethourK * 100 + setminK; // 开始时间
  int endtime = sethourG * 100 + setminG;   // 结束时间
  if (starttime == endtime)                 // 如果开始时间和结束时间是一样的话,就什么都不做
  {
    return;
  }

  int currtime = hour() * 100 + minute(); // 当前时间
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
        bLight = Minlight;        // 调小LED背光
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
        bLight = Minlight;         // 调小LED背光
      }
    }
  }
  ledcAnalogWrite(pwm_channel0, bLight); // 写入
}

void BluetoothProc()
{

  // 没有新连接时
  if (!deviceConnected && oldDeviceConnected)
  {
    // 给蓝牙堆栈准备数据的时间
    delay(500);
    pServer->startAdvertising();
    // 重新开始广播
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // 正在连接时
  if (deviceConnected && !oldDeviceConnected)
  {
    // 正在连接时进行的操作
    oldDeviceConnected = deviceConnected;
  }
}

// 创建一个函数，用来根据一个分割符来分割一个字符串
// 入口参数（目标字符串，分割符，返回的结果字符串数组） 返回分割数量
int StrSplit(String str, String fen, String *result)
{
  int index = 0, i = 0;
  // String temps[str.length()];
  if (str.length() == 0)
    return 0;
  Serial.println("目标字符串：" + str);
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
  Serial.print("匹配次数:");
  Serial.println(i);
  return i;
}

// //手工设置系统时间
// void MansetTime(int sc, int mn, int hr, int dy, int mt, int yr) {
//     // seconds, minute, hour, day, month, year $ microseconds(optional)
//     // ie setTime(20, 34, 8, 1, 4, 2021) = 8:34:20 1/4/2021
//     struct tm t = {0};        // Initalize to all 0's
//     t.tm_year = yr - 1900;    // This is year-1900, so 121 = 2021
//     t.tm_mon = mt - 1;
//     t.tm_mday = dy;
//     t.tm_hour = hr;
//     t.tm_min = mn;
//     t.tm_sec = sc;
//     time_t timeSinceEpoch = mktime(&t);
//     //   setTime(timeSinceEpoch, ms);
//     struct timeval now = { .tv_sec = timeSinceEpoch };
//     settimeofday(&now, NULL);
// }

