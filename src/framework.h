#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <Arduino.h>
#include <time.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#include <WiFi.h>
#include <Preferences.h>
#include <SPI.h>

#ifndef LittleFS
#include <SPIFFS.h>
#else
#include <LittleFS.h>
#endif
#include <Update.h>

#include <TFT_eSPI.h>
#include <OpenFontRender.h>

#include <HTTPClient.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#include <ArduinoLog.h>

#include <TLogPlus.h>

// using namespace TLogPlus;

#ifdef TELNET_LOGGING
#include <TelnetSerialStream.h>
using namespace TLogPlusStream;
TelnetSerialStream telnetSerialStream = TelnetSerialStream();
#endif

#ifdef WEBSTREAM_LOGGING
#include <WebSerialStream.h>
using namespace TLogPlusStream;
WebSerialStream webSerialStream = WebSerialStream();
#endif

#ifdef SYSLOG_LOGGING
#include <SyslogStream.h>
using namespace TLogPlusStream;
SyslogStream syslogStream = SyslogStream();
#endif

#ifdef MQTT_LOGGING
#include <MqttlogStream.h>
#include "main.h"
using namespace TLogPlusStream;
// EthernetClient client;
WiFiClient client;
MqttStream mqttStream = MqttStream(&client);
char topic[128] = "log/foo";
#endif

TFT_eSPI tft = TFT_eSPI(); // Create object "tft"
OpenFontRender ofr;
int screenWidth = tft.width();
int screenHeight = tft.height();
int screenCenterX = tft.width() / 2;
int screenCenterY = tft.height() / 2;
int topCenterY = tft.height() / 6;
int bottomCenterY = tft.height() * 5 / 6;
int middleCenterY = screenCenterY;
int leftCenterX = tft.width() / 4;
int rightCenterX = tft.width() * 3 / 4;

int chip_id = ESP.getEfuseMac();

#ifdef APP_NAME
const char *appName = APP_NAME;
#endif

int appInstanceID = -1;
char friendlyName[100] = "NoNameSet";

bool isFirstLoop = true;
bool isGoodTime = false;
bool isFirstDraw = true;

#ifdef NTP_SERVER
const char *ntpServer = NTP_SERVER;
#endif

// ********** Connectivity Parameters **********
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
TimerHandle_t checkFWUpdateTimer;
TimerHandle_t appInstanceIDWaitTimer;
TimerHandle_t wifiFailCountTimer;

int wifiFailCount = 0;

int volume = 50; // Volume is %
int bootCount = 0;
esp_sleep_wakeup_cause_t wakeup_reason;
esp_reset_reason_t reset_reason;
int maxOtherIndex = -1;

Preferences preferences;

#ifdef HOSTNAME
String hostname = HOSTNAME;
#endif

uint8_t macAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// MQTT Topics (25 character limit per level)
char onlineTopic[100];
char willTopic[100];
char appSubTopic[100];

char latestFirmwareFileName[100];

// **************** Debug Parameters ************************
String methodName = "";

bool isNullorEmpty(char *str)
{
    if ((str == NULL) || (str[0] == '\0'))
        return true;
    else
        return false;
}

bool isNullorEmpty(String str)
{
    return isNullorEmpty(str.c_str());
}

// check a string to see if it is numeric
bool isNumeric(char *str)
{
    for (byte i = 0; str[i]; i++)
    {
        if (!isDigit(str[i]))
            return false;
    }
    return true;
}

void clearScreen()
{
    tft.fillScreen(TFT_BLACK);
}

void drawString(String text, int x, int y)
{
    ofr.setCursor(x, y);
    if (ofr.getAlignment() == Align::MiddleCenter)
    {
        ofr.setCursor(x, y - ofr.getFontSize() / 5 - (ofr.getFontSize() > 36 ? 2 : 1));
    }
    ofr.printf(text.c_str());
}

void drawString(String text, int x, int y, int font_size)
{
    ofr.setFontSize(font_size);
    drawString(text, x, y);
}

void drawString(String text, int x, int y, int font_size, int color)
{
    ofr.setFontColor(color);
    drawString(text, x, y, font_size);
}

void drawString(String text, int x, int y, int font_size, int color, int bg_color)
{
    ofr.setFontColor(color, bg_color);
    drawString(text, x, y, font_size);
}

#endif // FRAMEWORK_H