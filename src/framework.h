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

#include <HTTPClient.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#include <ArduinoLog.h>
// #define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_LEVEL LOG_LEVEL_VERBOSE
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

int chip_id = ESP.getEfuseMac();

#ifdef APP_NAME
const char *appName = APP_NAME;
#endif

int appInstanceID = -1;

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




#endif // FRAMEWORK_H