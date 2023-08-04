#ifndef _WEATHERWARN_H_
#define _WEATHERWARN_H_
 
#include <Arduino.h>
#include <ArduinoJson.h>
 
class WeatherWarn {
  public:
    WeatherWarn();
    void config(String userKey, String location);//获取城市的天气
    void config_Grid(String userKey, String Grid_location);//获取经纬度地点的天气
    bool get();
    String getServerCode();
    String getLastUpdate();
    String getSender();
    String getPubTime();
    int getType();
    String getWeatherText();
    String getTitle();
    String getColor();
    String getTypeName();
    String getStatus();
 
  private:
    void _parseNowJson(char* input, size_t inputLength);  // 解析json信息
    String _url;
    String _response_code =  "no_init";            // API状态码
    String _last_update_str = "no_init";           // 当前API最近更新时间
    String _warn_sender_str = "no_init";           // 预警发布单位
    String _warn_pubTime_str = "no_init";          // 预警发布时间
    int _warn_type_int = 999;                      // 预警类型ID
    String _warn_text_str = "no_init";             // 预警详细文字描述
    String _warn_title_str = "no_init";            // 预警信息标题
    String _warn_severityColor_str = "no_init";    // 预警严重等级颜色
    String _warn_typeName_str = "no_init";         // 预警类型名称
    String _warn_status_str = "no_init";           // 预警信息的发布状态
};
 
#endif
