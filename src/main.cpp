
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
//#define LOG_LEVEL LOG_LEVEL_VERBOSE

// #define TELNET_LOGGING
// #define WEBSTREAM_LOGGING
// #define SYSLOG_LOGGING
// #define MQTT_LOGGING

#ifndef FRAMEWORK_H
#include "framework.h"
#endif

// **************** Debug Parameters ************************
String methodName = "";

// ********* App Parameters *****************

int appVersion = 1;
const char *appSecret = "536CB6A57A55C82BEDD22A9566A47";

int fontNum = 0;
bool isFirstLoop = true;
bool isGoodTime = false;

// For US Pacific Time Zone
const char *localTZ = "PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00";
const long gmtOffset_sec = -8 * 60 * 60;
const int daylightOffset_sec = 3600;

// ********** Connectivity Parameters **********

typedef void (*mqttMessageHandler)(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

int maxWifiFailCount = 5;
int wifiFailCountTimeLimit = 10;

// ********** App Global Variables **********

// Should be /internal/iot/firmware
const char *firmwareUrl = "/firmware/";
const char *appRootUrl = "/internal/iot/";

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

  Serial.begin(115200);
  Serial.println("Starting....");

#ifdef TELNET
  Log.addPrintStream(std::make_shared<TelnetSerialStream>(telnetSerialStream));
#endif

#ifdef WEBSTREAM
  Log.addPrintStream(std::make_shared<WebSerialStream>(webSerialStream));
#endif

#ifdef SYSLOG_LOGGING
  syslogStream.setDestination(SYSLOG_HOST);
  syslogStream.setRaw(false); // wether or not the syslog server is a modern(ish) unix.
#ifdef SYSLOG_PORT
  syslogStream.setPort(SYSLOG_PORT);
#endif
  Log.addPrintStream(std::make_shared<SyslogStream>(syslogStream));
#endif

#ifdef MQTT_HOST
  // mqttStream.setServer(MQTT_HOST);
  // mqttStream.setTopic(topic);
  // Log.addPrintStream(std::make_shared<MqttStream>(mqttStream));
#endif

  TLogPlus::Log.begin();

  Log.begin(LOG_LEVEL, &TLogPlus::Log, false);
  Log.setPrefix(printTimestamp);

  esp_base_mac_addr_get(macAddress);
  logMACAddress(macAddress);

  Log.noticeln("Starting %s v%d...", appName, appVersion);

  Log.verboseln("Entering ...");

  preferences.begin(appName, false);
  loadPrefs();
  if (appInstanceID < 0)
  {
    Log.infoln("AppInstanceID not set yet.");
  }
  else
  {
    Log.infoln("AppInstanceID: %d", appInstanceID);
  }

  bootCount++;

  Log.infoln("Boot count: %d", bootCount);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  reset_reason = esp_reset_reason();
  print_wakeup_reason();

  initFS();

  setupDisplay();

  // Add some custom code here
  initAppStrings();

  // Configure Hardware
  Log.infoln("Configuring hardware.");
  // pinMode(DOORBELL_PIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(DOORBELL_PIN), doorbellPressed, FALLING);

  // This is connectivity setup code
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
#ifdef MQTT_USER
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);
#endif

  if (appInstanceID >= 0)
  {
    mqttClient.onMessage(onMqttMessage);
  }
  else
  {
    mqttClient.onMessage(onMqttIDMessage);
    appInstanceIDWaitTimer = xTimerCreate("appInstanceIDWaitTimer", pdMS_TO_TICKS(10000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(setAppInstanceID));
    xTimerStart(appInstanceIDWaitTimer, 0);
  }

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
      return;
    }

    // put your main code here, to run repeatedly:
    if ((millis() % 10000) == 0)
    {
      logTimestamp();
      tft.fillScreen(TFT_BLACK);
      tft.setTextFont(fontNum++);
      if (fontNum > 7)
        fontNum = 0;
      tft.drawString(appName, tft.width() / 2, tft.height() / 2);
    }
  }

}