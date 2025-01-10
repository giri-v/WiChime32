
#define APP_NAME "esp32FWApp"
#include <secrets.h>
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

#define LOG_LEVEL LOG_LEVEL_INFO
// #define LOG_LEVEL LOG_LEVEL_VERBOSE

// #define TELNET_LOGGING
// #define WEBSTREAM_LOGGING
// #define SYSLOG_LOGGING
// #define MQTT_LOGGING



#ifndef FRAMEWORK_H
#include "framework.h"
#endif

#include "../assets/fonts/Roboto_Regular32pt7b.h"

// **************** Debug Parameters ************************
String methodName = "";

// ********* Framework App Parameters *****************
// ********* Framework App Parameters *****************

int appVersion = 1;
const char *appSecret = "536CB6A57A55C82BEDD22A9566A47";

// ********** Connectivity Parameters **********

typedef void (*mqttMessageHandler)(char *topic, char *payload,
                                   AsyncMqttClientMessageProperties properties,
                                   size_t len, size_t index, size_t total);

int maxWifiFailCount = 5;
int wifiFailCountTimeLimit = 10;

// ********** App Global Variables **********
// ********** Connectivity Parameters **********

typedef void (*mqttMessageHandler)(char *topic, char *payload,
                                   AsyncMqttClientMessageProperties properties,
                                   size_t len, size_t index, size_t total);

int maxWifiFailCount = 5;
int wifiFailCountTimeLimit = 10;

// ********** App Global Variables **********
int fontNum = 0;
bool isFirstLoop = true;
bool isGoodTime = false;

// For US Pacific Time Zone
const char *localTZ = "PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00";
const long gmtOffset_sec = -8 * 60 * 60;
const int daylightOffset_sec = 3600;

const GFXfont *timeFont = &Roboto_Regular32pt7b;
const GFXfont *dateFont = &Roboto_Regular32pt7b;

// Should be /internal/iot/firmware
const char *firmwareUrl = "/firmware/";
const char *appRootUrl = "/internal/iot/";

char currentTime[6] = "00:00";
char meridian[3] = "AM";
char currentDate[11] = "01/01/2000";
char dayOfWeek[10] = "Monday";
char monthOfYear[10] = "January";

bool dateChanged = false;

// ********** Possible Customizations Start ***********

int otherAppTopicCount = 0;
char otherAppTopic[10][25];
void (*otherAppMessageHandler[10])(char *topic, JsonDocument &doc);

// ************ Customizeable Functions *************
#include "app_functions.h"

// ********** Function Declarations **********
#include "framework_functions.h"

void setup()
{
  String oldMethodName = methodName;
  methodName = "setup()";

  setupFramework();
  setupFramework();

  esp_base_mac_addr_get(macAddress);
  logMACAddress(macAddress);

  bootCount++;

  Log.infoln("Boot count: %d", bootCount);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  reset_reason = esp_reset_reason();
  print_wakeup_reason();
  // Add some custom code here
  initAppStrings();

  // Configure Hardware
  Log.infoln("Configuring hardware.");
  // pinMode(DOORBELL_PIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(DOORBELL_PIN), doorbellPressed, FALLING);

  connectToWifi();

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void loop()
{
  TLogPlus::Log.loop();

  if (isFirstLoop)
  {
    isFirstLoop = false;
    Log.infoln("First loop done.");
  }

  if ((millis() % 1000) == 0)
  {
    if (!isGoodTime)
    {
      if (!(isGoodTime = checkGoodTime()))
        Log.infoln("Time not set yet.");
    }

    // put your main code here, to run repeatedly:
    if (getNewTime())
    {
      drawTime();
    }

    if (dateChanged)
    {
      // Do something
      drawDate();
    }
  }
}