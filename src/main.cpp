// ******************************************************************
// ******************************************************************
// ****************** Framework Configuration ***********************
// ******************************************************************
// ******************************************************************

// ************ App Name ***************
#define APP_NAME "esp32FWApp"
#include <secrets.h>

#define LOG_LEVEL LOG_LEVEL_INFO
// #define LOG_LEVEL LOG_LEVEL_VERBOSE

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

  framework_setup();

  app_setup();

  framework_start();

  Log.verboseln("Exiting...");
  methodName = oldMethodName;
}

void loop()
{
  framework_loop();
  
  app_loop();
}