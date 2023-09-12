#ifndef _HTTPS_GET_UTILS_H_
#define _HTTPS_GET_UTILS_H_
 
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
 
class HttpsGetUtils {  
  public:
    HttpsGetUtils();
    static bool getString(const char* url, uint8_t*& outbuf, size_t &len);
    static const char  *host;		// 服务器地址
  private:
    static bool fetchBuffer(const char* url);
    static uint8_t _buffer[1024 * 3]; //gzip流最大缓冲区
    static size_t _bufferSize;
 
};
 
#endif
