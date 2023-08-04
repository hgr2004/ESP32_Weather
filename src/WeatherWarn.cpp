#include "WeatherWarn.h"
#include "HttpsGetUtils.h"
WeatherWarn::WeatherWarn() {
}
 
//例如 location="101010100"，城市相关ID从https://github.com/qwd/LocationList下载。
void WeatherWarn::config(String userKey, String location) {
    _url =  String(HttpsGetUtils::host) +  "/v7/warning/now?location=" + location + "&key=" + userKey + "&lang=zh";
}
 
//以英文逗号分隔的经度,纬度坐标（十进制，支持小数点后两位）例如Grid_location="116.41,39.92"
void WeatherWarn::config_Grid(String userKey, String Grid_location) {
    _url =  String(HttpsGetUtils::host) +  "/v7/warning/now?location=" + Grid_location +"&key=" + userKey + "&lang=zh";
}
 
bool WeatherWarn::get() {
   uint8_t *outbuf=NULL;
  size_t len=0;
  Serial.println("Get WeatherWarning..");
  bool result = HttpsGetUtils::getString(_url.c_str(), outbuf, len);
  if(outbuf && len){
      _parseNowJson((char*)outbuf,len);
  } else {
    Serial.println("Get WeatherWarning failed");
  }
  //一定要记得释放内存
  if(outbuf!=NULL) {
    free(outbuf);
    outbuf=NULL;
  }
  return result;
}
 
// 解析Json信息
void WeatherWarn::_parseNowJson(char* input, size_t inputLength) {
StaticJsonDocument<1024 * 2> doc;
DeserializationError error = deserializeJson(doc, input, inputLength);

  //JsonObject now = doc["warning"]["0"];
  JsonArray warn = doc["warning"];
  if(warn.isNull()){
    _warn_status_str = "no";
    return;
  }

  JsonObject now = warn[0];
 
  _response_code = doc["code"].as<String>();                    // API状态码
  _last_update_str = doc["updateTime"].as<String>();            // 当前API最近更新时间
  _warn_sender_str = now["sender"].as<String>();                   // 预警发布单位
  _warn_pubTime_str = now["pubTime"].as<String>();               // 预警发布时间 
  _warn_type_int = now["type"].as<int>();                        // 预警类型ID
  _warn_text_str = now["text"].as<String>();                     // 预警详细文字描述 
  _warn_title_str = now["title"].as<String>();                   // 预警信息标题 
  _warn_severityColor_str = now["severityColor"].as<String>();   // 预警严重等级颜色
  _warn_typeName_str = now["typeName"].as<String>();             // 预警类型名称
  _warn_status_str = now["status"].as<String>();                 // 预警信息的发布状态
}
 
// API状态码
String WeatherWarn::getServerCode() {
  return _response_code;
}
 
// 当前API最近更新时间
String WeatherWarn::getLastUpdate() {
  return _last_update_str;
}
 
// 实况温度
String WeatherWarn::getSender() {
  return _warn_sender_str;
}
 
// 实况体感温度
String WeatherWarn::getPubTime() {
  return _warn_pubTime_str;
}
 
// 当前天气状况和图标的代码
int WeatherWarn::getType() {
  return _warn_type_int;
}
 
// 实况天气状况的文字描述
String WeatherWarn::getWeatherText() {
  return _warn_text_str;
}
 
// 实况风向
String WeatherWarn::getTitle() {
  return _warn_title_str;
}
 
// 实况风力等级
String WeatherWarn::getColor() {
  return _warn_severityColor_str;
}
 
// 实况相对湿度百分比数值
String WeatherWarn::getTypeName() {
  return  _warn_typeName_str;
}
// 实况降水量,毫米
String WeatherWarn::getStatus() {
  return _warn_status_str;
}
