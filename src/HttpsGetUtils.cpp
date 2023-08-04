#include "HttpsGetUtils.h"
#include "ArduinoUZlib.h" // gzip库

uint8_t HttpsGetUtils::_buffer[1024 * 4];
const char *HttpsGetUtils::host = "https://devapi.qweather.com"; // 服务器地址，这是免费用户的地址，如果非免费用户，改为：https://api.qweather.com
size_t HttpsGetUtils::_bufferSize = 0;

HttpsGetUtils::HttpsGetUtils()
{
}

bool HttpsGetUtils::getString(const char *url, uint8_t *&outbuf, size_t &outlen)
{
    fetchBuffer(url); // HTTPS获取数据流
    if (_bufferSize)
    {
        // Serial.print("buffersize:");
        // Serial.println(_bufferSize, DEC);
        ArduinoUZlib::decompress(_buffer, _bufferSize, outbuf, outlen); // GZIP解压
        _bufferSize = 0;
        return true;
    }
    return false;
}

bool HttpsGetUtils::fetchBuffer(const char *url)
{
    _bufferSize = 0;
    std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    if (https.begin(*client, url))
    {
        https.addHeader("Accept-Encoding", "gzip");
        https.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/115.0");

        int httpCode = https.GET();
        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK)
            {
                int len = https.getSize(); // get length of document (is -1 when Server sends no Content-Length header)
                static uint8_t buff[128] = {0}; // create buffer for read
                int offset = 0;                 // read all data from server
                while (https.connected() && (len > 0 || len == -1))
                {
                    size_t size = client->available(); // get available data size
                    if (size)
                    {
                        int c = client->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        memcpy(_buffer + offset, buff, sizeof(uint8_t) * c);
                        offset += c;
                        if (len > 0)
                        {
                            len -= c;
                        }
                    }
                    delay(1);
                }
                _bufferSize = offset;
            }
        }
        else
        {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
    }
    else
    {
        Serial.printf("Unable to connect\n");
    }
    return _bufferSize > 0;
    ;
}
