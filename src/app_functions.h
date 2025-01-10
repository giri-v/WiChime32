#ifndef APP_FUNCTIONS_H
#define APP_FUNCTIONS_H

#include "framework.h"

void printTimestamp(Print *_logOutput, int x);
void logTimestamp();
void storePrefs();
void loadPrefs();
void setAppInstanceID();
void ProcessMqttDisconnectTasks();
void ProcessMqttConnectTasks();
void ProcessWifiDisconnectTasks();
void ProcessWifiConnectTasks();
void appMessageHandler(char *topic, JsonDocument &doc);
void setupDisplay();
void initAppStrings();
void drawTime();

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

//////////////////////////////////////////
//// Customizable Functions
//////////////////////////////////////////
void setupDisplay()
{
    String oldMethodName = methodName;
    methodName = "setupDisplay()";
    Log.verboseln("Entering");

    Log.infoln("Setting up display.");
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(fontNum++);
    tft.setTextSize(2);

    tft.setTextDatum(MC_DATUM);

    tft.drawString(appName, tft.width() / 2, tft.height() / 2);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void initAppStrings()
{
    sprintf(onlineTopic, "%s/online", appName);
    sprintf(willTopic, "%s/offline", appName);

    sprintf(appSubTopic, "%s/#", appName);
}

void ProcessWifiConnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessAppWifiConnectTasks()";
    Log.verboseln("Entering...");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void ProcessWifiDisconnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessAppWifiDisconnectTasks()";
    Log.verboseln("Entering...");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void ProcessMqttConnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessMqttConnectTasks()";
    Log.verboseln("Entering...");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void ProcessMqttDisconnectTasks()
{
    String oldMethodName = methodName;
    methodName = "ProcessMqttDisconnectTasks()";
    Log.verboseln("Entering...");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void appMessageHandler(char *topic, JsonDocument &doc)
{
    String oldMethodName = methodName;
    methodName = "appMessageHandler()";
    Log.verboseln("Entering...");

    // Add your implementation here
    char topics[10][25];
    int topicCounter = 0;
    char *token = strtok(topic, "/");

    while ((token != NULL) && (topicCounter < 11))
    {
        strcpy(topics[topicCounter++], token);
        token = strtok(NULL, "/");
    }

    // We can assume the first 2 subtopics are the appName and the appInstanceID
    // The rest of the subtopics are the command

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return;
}

void setAppInstanceID()
{
    String oldMethodName = methodName;
    methodName = "setAppInstanceID()";
    Log.verboseln("Entering...");

    appInstanceID = maxOtherIndex + 1;
    storePrefs();

    Log.infoln("Got appInstanceID, restarting...");
    esp_restart();

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void loadPrefs()
{
    String oldMethodName = methodName;
    methodName = "loadPrefs()";
    Log.verboseln("Entering...");

    bool doesExist = preferences.isKey("appInstanceID");
    if (doesExist)
    {
        Log.infoln("Loading settings.");
        appInstanceID = preferences.getInt("appInstanceID");
        volume = preferences.getInt("Volume");
        bootCount = preferences.getInt("BootCount");
        // enableSnapshot = preferences.getBool("EnableSnapshot");
    }
    else
    {
        Log.warningln("Could not find Preferences!");
        Log.noticeln("appInstanceID not set yet!");
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

    preferences.putInt("appInstanceID", appInstanceID);
    preferences.putInt("Volume", volume);
    preferences.putInt("BootCount", bootCount);
    // preferences.putBool("EnableSnapshot", enableSnapshot);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void logTimestamp()
{
    String oldMethodName = methodName;
    methodName = "logTimestamp()";
    Log.verboseln("Entering...");

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

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
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

void drawTime()
{
    String oldMethodName = methodName;
    methodName = "drawTime()";
    Log.verboseln("Entering...");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Log.errorln("Failed to obtain time");
        Log.verboseln("Exiting...");
        methodName = oldMethodName;
        return;
    }

    char c[20];
    strftime(c, 20, "%I:%M", &timeinfo);

    char meridian[3];
    strftime(meridian, 3, "%p", &timeinfo);

    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(c, tft.width() / 2, tft.height() / 2);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

IRAM_ATTR void interruptService()
{
}
#endif // APP_FUNCTIONS_H