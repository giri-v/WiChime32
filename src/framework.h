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

#define APP_KEY ""
#define APP_SECRET "536CB6A57A55C82BEDD22A9566A47"

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

#include <ESPAsyncWebServer.h>
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
int topCenterY = tft.height() / 6;
int bottomCenterY = tft.height() * 5 / 6;
int middleCenterY = screenCenterY;
int leftCenterX = tft.width() / 4;
int rightCenterX = tft.width() * 3 / 4;
PNG png;
int32_t xPos = 0;
int32_t yPos = 0;

#ifdef APP_NAME
const char *appName = APP_NAME;
#endif

const char *appSecret = APP_SECRET;
#define FIRMWARE_VERSION "v0.0.1"

int appInstanceID = -1;
char friendlyName[100] = "NoNameSet";

bool isFirstLoop = true;
bool isGoodTime = false;
bool isFirstDraw = true;

#ifdef NTP_SERVER
const char *ntpServer = NTP_SERVER;
#endif

// ********** Connectivity Parameters **********
AsyncWebServer webServer(80);
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
bool shouldReboot = false;
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

// Here are the callback functions that the decPNG library
// will use to open files, fetch data and close the file.

File pngfile;

void *pngOpen(const char *filename, int32_t *size)
{
    Log.verboseln("Attempting to open %s\n", filename);
    pngfile = SPIFFS.open(filename, "r");
    *size = pngfile.size();
    return &pngfile;
}

void pngClose(void *handle)
{
    File pngfile = *((File *)handle);
    if (pngfile)
        pngfile.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
    if (!pngfile)
        return 0;
    page = page; // Avoid warning
    return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
    if (!pngfile)
        return 0;
    page = page; // Avoid warning
    return pngfile.seek(position);
}

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

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes)
{
    if (bytes < 1024)
        return String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        return String(bytes / 1024.0 / 1024.0) + " MB";
    else
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

#pragma endregion



#pragma region File System

void initFS()
{
#ifndef LittleFS
    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Log.errorln("An Error has occurred while mounting SPIFFS");
        return;
    }
#else
    if (!LittleFS.begin())
    {
        Log.errorln("Flash FS initialisation failed!");
        while (1)
            yield(); // Stay here twiddling thumbs waiting
    }
#endif
    Log.infoln("Flash FS available!");
}

#pragma endregion

#pragma region Display and Drawing Functions

#ifdef USE_GRAPHICS

#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(); // Create object "tft"

int screenWidth = tft.width();
int screenHeight = tft.height();
int screenCenterX = tft.width() / 2;
int screenCenterY = tft.height() / 2;

void clearScreen()
{
    tft.fillScreen(TFT_BLACK);
}

#endif

#pragma endregion

#pragma region OpenFontRenderer Drawing Functions

#ifdef USE_OPEN_FONT_RENDERER

#include <OpenFontRender.h>

OpenFontRender ofr;

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

#endif

#pragma endregion

#pragma region SD Card Functions

#ifdef USE_SD_CARD
#include <SD.h>

#define SD_CS 5

void initSD()
{
    String oldMethodName = methodName;
    methodName = "initSD()";
    Log.verboseln("Entering...");

    if (!SD.begin(SD_CS))
    {
        Log.errorln("SD Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        Log.errorln("No SD card attached");
        return;
    }
    Log.infoln("SD Card Type: %s", cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SDSC"
                                                              : cardType == CARD_SDHC ? "SDHC"
                                                                                      : "UNKNOWN");

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Log.infoln("SD Card Size: %lluMB", cardSize);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}
#endif

#pragma endregion

#pragma region Audio Functions

#ifdef USE_AUDIO
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
bool mp3Done = true;

/// @fn void initAudioOutput()
/// @brief The only function to 

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
        Log.errorln("Failed to play MP3!");
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
}

void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    static uint16_t dmaBuffer[MAX_IMAGE_WIDTH]; // static so buffer persists after fn exit

    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(xPos, yPos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void drawPNG(const char *filename, int x, int y)
{

    int16_t rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    xPos = x;
    yPos = y;
    if (rc == PNG_SUCCESS)
    {
        tft.startWrite();
        Log.verboseln("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        uint32_t dt = millis();
        if (png.getWidth() > MAX_IMAGE_WIDTH)
        {
            Serial.println("Image too wide for allocated lin buffer!");
        }
        else
        {
            rc = png.decode(NULL, 0);
            png.close();
        }
        tft.endWrite();
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

int webGet(String req, String &res)
{
    String oldMethodName = methodName;
    methodName = "webGet(String req, String &res)";

    int result = -1;

    Log.verboseln("Connecting to http://%s:%d%s", HTTP_SERVER, HTTP_PORT,
                  req.c_str());

    WiFiClient client;
    HTTPClient http;
    String payload;
    Log.verboseln("[HTTP] begin...");

    // int connRes = client.connect(IPAddress(192,168,0,12), 5000);
    // Log.verboseln("Connected: %d", connRes);

    // if (http.begin(client, req))
    if (http.begin(client, HTTP_SERVER, HTTP_PORT, req))
    { // HTTP

        Log.verboseln("[HTTP] GET...");
        // start connection and send HTTP header
        int httpCode = http.GET();
        result = httpCode;

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has
            // been handled
            Log.verboseln("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                res = http.getString();
            }
        }
        else
        {
            Log.errorln("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
        }

        http.end();
    }
    else
    {
        Log.warningln("[HTTP] Unable to connect");
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return result;
}
#endif // FRAMEWORK_H