
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

//#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_LEVEL LOG_LEVEL_VERBOSE

// #define TELNET_LOGGING
// #define WEBSTREAM_LOGGING
// #define SYSLOG_LOGGING
// #define MQTT_LOGGING

#ifndef FRAMEWORK_H
#include "framework.h"
#endif

// ************ Customizeable Functions *************
#include "app_functions.h"

// ********** Function Declarations **********
#include "framework_functions.h"

void setup()
{
  String oldMethodName = methodName;
  methodName = "setup()";

  setupFramework();

  app_setup();

  connectToWifi();

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void loop()
{
  TLogPlus::Log.loop();

  app_loop();
}