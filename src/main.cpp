#include <Arduino.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#define APP_NAME "esp32FWApp"

#include <secrets.h>
// #define TELNET_LOGGING

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
/*
#define ST7796_DRIVER
#define TFT_WIDTH 320
#define TFT_HEIGHT 480
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS -1
#define TFT_DC 0
#define TFT_RST -1
*/
/*
#define ILI9341_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_BL 21             // LED back-light control pin
#define TFT_BACKLIGHT_ON HIGH // Level to turn ON back-light (HIGH or LOW)

#define TFT_MISO 12
#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 14
#define TFT_CS 15  // Chip select control pin
#define TFT_DC 2   // Data Command control pin
#define TFT_RST 12 // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL 21  // LED back-light

#define TOUCH_CS 33 // Chip select pin (T_CS) of touch screen
*/

#include <HTTPClient.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#include <ArduinoLog.h>
#define LOG_LEVEL LOG_LEVEL_INFO
#include <TLogPlus.h>

#ifndef SECRETS_H
#define SECRETS_H

#define HOSTNAME "myESP8266";
#define NTP_SERVER "pool.ntp.org"

#define WIFI_SSID "APName"
#define WIFI_PASSWORD "APPassword"

#define MQTT_HOST IPAddress(192, 168, 0, 200)
#define MQTT_PORT 1883

#define LATITUDE 37.3380937
#define LONGITUDE -121.8853892

#endif // SECRETS_H

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
using namespace TLogPlusStream;
// EthernetClient client;
WiFiClient client;
MqttStream mqttStream = MqttStream(&client);
char topic[128] = "log/foo";
#endif

TFT_eSPI tft = TFT_eSPI(); // Create object "tft"

int chip_id = ESP.getEfuseMac();

// **************** Debug Parameters ************************
String methodName = "";

// ********* App Parameters *****************
const char *appName = APP_NAME;
int appID = -1;

int volume = 50; // Volume is %
int bootCount = 0;
esp_sleep_wakeup_cause_t wakeup_reason;
esp_reset_reason_t reset_reason;
int maxOtherIndex = -1;

Preferences preferences;

// ********** Time/NTP Parameters **********
const char *ntpServer = NTP_SERVER;
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

const char *localTZ = "PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00";

// ********** Connectivity Parameters **********
String hostname = HOSTNAME;

char ipAddress[100];
char port[25];
uint8_t macAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66}; // TODO: make this a meaningful default
uint8_t wifiSTAMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};
uint8_t wifiAPMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};
uint8_t btMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};
uint8_t ethMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
TimerHandle_t checkFWUpdateTimer;
TimerHandle_t appIDWaitTimer;
// WiFiEventHandler wifiConnectHandler;
// WiFiEventHandler wifiDisconnectHandler;

// MQTT Topics (25 character limit per level)
char onlineTopic[100];
char willTopic[100];
char idTopic[100];
char snapshotTopic[100];

char appSubTopic[100];

// put function declarations here:
void loadPrefs();
void storePrefs();
void mqttPublishID();
int myFunction(int, int);

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

void loadPrefs()
{
  String oldMethodName = methodName;
  methodName = "loadPrefs()";
  Log.verboseln("Entering...");

  bool doesExist = preferences.isKey("appID");
  if (doesExist)
  {
    Log.infoln("Loading settings.");
    appID = preferences.getInt("appID");
    volume = preferences.getInt("Volume");
    bootCount = preferences.getInt("BootCount");
    // enableSnapshot = preferences.getBool("EnableSnapshot");
  }
  else
  {
    Log.warningln("Could not find Preferences!");
    Log.noticeln("appID not set yet!");
  }

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void storePrefs()
{
  String oldMethodName = methodName;
  methodName = "storePrefs()";
  Log.verboseln("Entering...");

  Log.infoln("Storing Preferences.");

  preferences.putInt("appID", appID);
  preferences.putInt("Volume", volume);
  preferences.putInt("BootCount", bootCount);
  // preferences.putBool("EnableSnapshot", enableSnapshot);

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void logTimestamp()
{
  char c[20];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  if (timeinfo->tm_year == 70)
  {
    sprintf(c, "%10lu ", millis());
  }
  else
  {
    strftime(c, 20, "%Y%m%d %H:%M:%S", timeinfo);
  }

  Log.infoln("Time: %s", c);
}

void printTimestamp(Print *_logOutput, int x)
{
  char c[20];
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  if (timeinfo->tm_year == 70)
  {
    sprintf(c, "%10lu ", millis());
  }
  else
  {
    strftime(c, 20, "%Y%m%d %H:%M:%S", timeinfo);
  }
  _logOutput->print(c);
  _logOutput->print(": ");
  _logOutput->print(methodName);
  _logOutput->print(": ");
}

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
  Log.infoln("Flash FS available!");

#endif
}

void setupDisplay()
{
  String oldMethodName = methodName;
  methodName = "setupDisplay()";
  Log.verboseln("Entering");

  Log.infoln("Setting up display.");
  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  // delayMicroseconds(1000);
  tft.fillScreen(TFT_BLUE);
  // delayMicroseconds(1000);
  tft.fillScreen(TFT_BLACK);

  // tft.setFont(baseFont);

  tft.setTextFont(7);
  tft.setTextSize(1);

  tft.setTextDatum(BC_DATUM);

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void initAppStrings()
{
  sprintf(onlineTopic, "%s/online", appName);
  sprintf(willTopic, "%s/offline", appName);
  sprintf(idTopic, "%s/id", appName);

  sprintf(appSubTopic, "%s/#", appName);
}

void initAppInstanceStrings()
{
  sprintf(onlineTopic, "%s/status", appName);
  sprintf(willTopic, "%s/offline", appName);
  sprintf(idTopic, "%s/id", appName);

  sprintf(appSubTopic, "%s/#", appName);
}

void connectToWifi()
{
  String oldMethodName = methodName;
  methodName = "connectToWifi()";

  Log.infoln("Connecting...");
  WiFi.hostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  methodName = oldMethodName;
}

void connectToMqtt()
{
  String oldMethodName = methodName;
  methodName = "connectToMqtt()";

  Log.infoln("Connecting...");
  mqttClient.connect();

  methodName = oldMethodName;
}

void onWifiConnect(const WiFiEvent_t &event)
{
  String oldMethodName = methodName;
  methodName = "onWifiConnect(const WiFiEventStationModeGotIP &event)";
  Log.verboseln("Entering...");

  (void)event;

  Log.infoln("Connected to Wi-Fi. IP address: %p", WiFi.localIP());
  Log.infoln("Connecting to NTP Server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // configTime(localTZ, ntpServer);
  Log.infoln("Connected to NTP Server!");
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  char tim[20];
  strftime(tim, 20, "%d/%m/%Y %H:%M:%S", timeinfo);

  // bForecastChanged = true;
  Log.infoln("Local Time: %s", tim);

  Log.infoln("Connecting to MQTT Broker...");
  connectToMqtt();

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void onWifiDisconnect(const WiFiEvent_t &event)
{
  String oldMethodName = methodName;
  methodName = "onWifiDisconnect(const WiFiEventStationModeDisconnected &event)";
  Log.verboseln("Entering...");

  (void)event;

  Log.infoln("Disconnected from Wi-Fi.");
  Log.infoln("Disconnecting mqttReconnectTimer");
  xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  // mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  Log.infoln("Reconnecting to WiFi...");
  xTimerStart(wifiReconnectTimer, 0);
  // wifiReconnectTimer.once(2, connectToWifi);

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void WiFiEvent(WiFiEvent_t event)
{
  String oldMethodName = methodName;
  methodName = "WiFiEvent(WiFiEvent_t event)";
  Log.verboseln("Entering...");

  switch (event)
  {
  case ARDUINO_EVENT_WIFI_READY:
    Log.infoln("Wifi ready.");
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    Log.infoln("Wifi station start.");
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Log.infoln("Wifi station connected.");
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    onWifiConnect(event);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    onWifiDisconnect(event);
    break;
  }

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void mqttPublishID()
{
  String oldMethodName = methodName;
  methodName = "mqttPublishID()";
  Log.verboseln("Entering...");

  char onlineTopic[20];
  char payloadJson[20];
  sprintf(onlineTopic, "%s/online", appName);
  sprintf(payloadJson, "%i", appID);
  Log.infoln("Published %s topic", onlineTopic);
  int pubRes = mqttClient.publish(onlineTopic, 1, false, payloadJson);

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void mqttPublishWill()
{
  String oldMethodName = methodName;
  methodName = "mqttPublishID()";
  Log.verboseln("Entering...");

  char offlineTopic[20];
  char payloadJson[20];
  sprintf(offlineTopic, "%s/offline", appName);
  sprintf(payloadJson, "%i", appID);
  Log.infoln("Published Last Will and Testament %s", offlineTopic);
  mqttClient.publish(offlineTopic, 1, true, payloadJson);

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void onMqttConnect(bool sessionPresent)
{
  String oldMethodName = methodName;
  methodName = "onMqttConnect(bool sessionPresent)";
  Log.verboseln("Entering...");

  Log.infoln("Connected to MQTT broker: %p , port: %d", MQTT_HOST, MQTT_PORT);
  Log.infoln("Session present: %T", sessionPresent);

  uint16_t packetIdSub1 = mqttClient.subscribe(appSubTopic, 2);
  if (packetIdSub1 > 0)
    Log.infoln("Subscribing to %s at QoS 2, packetId: %u", appSubTopic, packetIdSub1);
  else
    Log.errorln("Failed to subscribe to %s!!!", appSubTopic);

  if (appID > -1)
    mqttPublishID();
  else
  {
    // mqttClient.publish(idTopic, 1, false);
    Log.infoln("Don't have appID yet so nothing to publish to MQTT.");
    // wichimeIDWaitTimer.once_ms(10000, registerMQTTTopics);
  }
  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  String oldMethodName = methodName;
  methodName = "onMqttConnect(bool sessionPresent)";
  Log.verboseln("Entering...");

  (void)reason;

  Log.warningln("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void logMQTTMessage(char *topic, int len, char *payload)
{
  String oldMethodName = methodName;
  methodName = "logMQTTMessage(char *topic, int len, char *payload)";
  Log.infoln("[%s] {%s} ", topic, payload);
  /*
    Log.infoln("Payload Length: %d", len);

    if (!isNullorEmpty(payload))
    {
      Log.infoln("Payload: %s", payload);
    }
  */
  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void onMqttIDMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String oldMethodName = methodName;
  methodName = "onMqttIDMessage()";
  Log.verboseln("Entering...");

  char *topics[10];
  int topicCounter = 0;
  char *token = strtok(topic, "/");

  while (token != NULL)
  {
    strcpy(topics[topicCounter++], token);
    token = strtok(NULL, "/");
  }

  if (strcmp(topic, appName) == 0) // Handle all wichime messages
  {
    if ((strstr(topic, "offline") != NULL) || (strstr(topic, "online") != NULL))
    {
      int otherIndex = atoi(payload);
      if (otherIndex >= 0)
        maxOtherIndex = max(maxOtherIndex, otherIndex);
    }
  }

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void doUpdateFirmware()
{
  File file = SPIFFS.open("/new.bin");

  if (!file)
  {
    Log.errorln("Failed to open file for reading");
    return;
  }

  Log.infoln("Starting update..");

  size_t fileSize = file.size();

  if (!Update.begin(fileSize))
  {

    Log.warningln("Cannot do the update!");
    return;
  };

  Update.writeStream(file);

  if (Update.end())
  {

    Log.infoln("Successful update");
  }
  else
  {

    Log.errorln("Error Occurred: %s", String(Update.getError()));
    return;
  }

  file.close();


  Log.infoln("Reset in 2 seconds...");
  delay(2000);

  ESP.restart();
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

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String oldMethodName = methodName;
  methodName = "onMqttMessage()";
  Log.verboseln("Entering...");

  logMQTTMessage(topic, len, payload);

  return;

  char *topics[10];
  int topicCounter = 0;
  char *token = strtok(topic, "/");

  while ((token != NULL) && (topicCounter < 11))
  {
    strcpy(topics[topicCounter++], token);
    token = strtok(NULL, "/");
  }

  if (topicCounter > 10)
  {
    Log.noticeln("MQTT Topic has > 10 levels.");
    return;
  }

  if ((strcmp(topic, appName) == 0) && (topicCounter > 1))
  {
    // Handle all messages for our App (AppName is in topic #1)
    if (isNumeric(topics[1]))
    {
      // This is a command because there is an AppID in topic #2
      int cmdTargetID = atoi(topics[1]);
      if (cmdTargetID == appID)
      { // This command is for us (AppID == our appID)
      }
      else
      { // This command is for another instance
      }
    }
    else
    { // This is not a command
    }
  }
  else if (strcmp(topic, "otherThingTopic") == 0)
  {
    // This is another App's message we are interested in
    Log.infoln("Got CallerID topic");

    char msg[len + 1];
    memcpy(msg, payload, len);
    msg[len] = 0;
    logMQTTMessage(topic, len, msg);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);

    // Test if parsing succeeds.
    if (error)
    {
      Log.errorln("deserializeJson() failed: %s", error.c_str());
    }
    else
    {
    }
  }
  else // This is an unhandled topic
  {
  }

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void logMACAddress(uint8_t baseMac[6])
{
  char mac[200];
  sprintf(mac, "MAC Address: { %02x:%02x:%02x:%02x:%02x:%02x }",
          baseMac[0], baseMac[1], baseMac[2],
          baseMac[3], baseMac[4], baseMac[5]);
  Log.infoln(mac);
}

void readMACs()
{
  // Get MAC address of the WiFi station interface

  esp_read_mac(wifiSTAMACAddress, ESP_MAC_WIFI_STA);
  // Get the MAC address of the Wi-Fi AP interface
  esp_read_mac(wifiAPMACAddress, ESP_MAC_WIFI_SOFTAP);
  // Get the MAC address of the Bluetooth interface
  esp_read_mac(btMACAddress, ESP_MAC_BT);
  // Get the MAC address of the Ethernet interface
  esp_read_mac(ethMACAddress, ESP_MAC_ETH);
}

void print_wakeup_reason()
{

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Log.infoln("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Log.infoln("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Log.infoln("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Log.infoln("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Log.infoln("Wakeup caused by ULP program");
    break;
  default:
    Log.infoln("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}


void setAppID()
{
  String oldMethodName = methodName;
  methodName = "setAppID()";
  Log.verboseln("Entering...");

  appID = maxOtherIndex + 1;
  storePrefs();

  Log.infoln("Got appID, restarting...");
  esp_restart();

  /*
    mqttPublishID();
    Log.infoln("About to change onMessage handler.");
    mqttClient.onMessage(NULL);
    Log.infoln("Changed onMessage handler to NULL.");
    mqttClient.onMessage(onMqttMessage);
    Log.infoln("Changed onMessage handler.");
    xTimerDelete(appIDWaitTimer, 0);
  */
  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

int webGet(String req, String &res)
{
  String oldMethodName = methodName;
  methodName = "webGet(String req, String &res)";

  int result = -1;

  WiFiClient client;
  HTTPClient http;
  String payload;
  Log.verboseln("[HTTP] begin...");

  if (http.begin(client, req))
  { // HTTP

    Log.verboseln("[HTTP] GET...");
    // start connection and send HTTP header
    int httpCode = http.GET();
    result = httpCode;

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
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

  methodName = oldMethodName;
  return result;
}

void checkFWUpdate()
{
  String oldMethodName = methodName;
  methodName = "checkFWUpdate()";

  Log.infoln("Completed checking for FW updates.");
  xTimerDelete(checkFWUpdateTimer, 0);

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

IRAM_ATTR void interruptService()
{
}

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

  Log.noticeln("Starting %s...", appName);
  Log.verboseln("Entering ...");

  preferences.begin(appName, false);
  loadPrefs();
  bootCount++;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  reset_reason = esp_reset_reason();
  // initFS();

  setupDisplay();

  // Add some custom code here
  initAppStrings();

  // Configure Hardware
  Log.infoln("Configuring hardware.");
  // pinMode(DOORBELL_PIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(DOORBELL_PIN), doorbellPressed, FALLING);

  Log.verboseln("MOSI: %i", MOSI);
  Log.verboseln("MISO: %i", MISO);
  Log.verboseln("SCK: %i", SCK);
  Log.verboseln("SS: %i", SS);

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

  if (appID >= 0)
  {
    mqttClient.onMessage(onMqttMessage);
    checkFWUpdateTimer = xTimerCreate("checkFWUpdateTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(checkFWUpdate));
  }
  else
  {
    mqttClient.onMessage(onMqttIDMessage);
    appIDWaitTimer = xTimerCreate("appIDWaitTimer", pdMS_TO_TICKS(10000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(setAppID));
    xTimerStart(appIDWaitTimer, 0);
  }

  connectToWifi();

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void loop()
{
  TLogPlus::Log.loop();

  // put your main code here, to run repeatedly:
  if ((millis() % 10000) == 0)
  {
    logTimestamp();
  }
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}