#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <Arduino.h>
#include <time.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#ifndef SECRETS_H
#define SECRETS_H

#define HOSTNAME "myESP8266";
#define NTP_SERVER "pool.ntp.org"

#define WIFI_SSID "APName"
#define WIFI_PASSWORD "APPassword"

#define HTTP_SERVER "192.168.0.12"
#define HTTP_PORT 5000

#define MQTT_HOST IPAddress(192, 168, 0, 200)
#define MQTT_PORT 1883

#define LATITUDE 37.3380937
#define LONGITUDE -121.8853892

#endif // SECRETS_H
#include <WiFi.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>

#ifndef LittleFS
#include <SPIFFS.h>
#else
#include <LittleFS.h>
#endif
#include <Update.h>

#define SD_CS 5

#include <TFT_eSPI.h>
#include <OpenFontRender.h>
#include <PNGdec.h>
#define MAX_IMAGE_WIDTH 320

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

#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
bool mp3Done = true;

TFT_eSPI tft = TFT_eSPI(); // Create object "tft"
OpenFontRender ofr;
int screenWidth = tft.width();
int screenHeight = tft.height();
int screenCenterX = tft.width() / 2;
int screenCenterY = tft.height() / 2;

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

void initAudioOutput()
{
    out = new AudioOutputI2S(0, 2, 8, -1); // Output to builtInDAC
    out->SetOutputModeMono(true);
    out->SetGain(1.0);
}


void playMP3(char *filename)
{
    AudioFileSourceSD *file;
    AudioFileSourceID3 *id3;

    file = new AudioFileSourceSD(filename);
    id3 = new AudioFileSourceID3(file);
    mp3 = new AudioGeneratorMP3();
    if (!mp3->begin(id3, out))
    {
        Log.errorln("Failed to begin MP3 decode.");
    }
    else
    {
        mp3Done = false;
    }
}

void playMP3Loop()
{
    if (mp3->isRunning())
    {
        if (!mp3->loop())
        {
            mp3->stop();
            mp3Done = true;
        }
    }
    else
    {
        mp3Done = true;
    }
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
        ofr.setCursor(x, y - ofr.getFontSize() / 5 - 2);
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