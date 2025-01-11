#ifndef APP_FUNCTIONS_H
#define APP_FUNCTIONS_H

#include "framework.h"

#include "../assets/fonts/Roboto.h"

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

// For US Pacific Time Zone
const char *localTZ = "PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00";
const long gmtOffset_sec = -8 * 60 * 60;
const int daylightOffset_sec = 3600;

// Should be /internal/iot/firmware
const char *firmwareUrl = "/firmware/";
const char *appRootUrl = "/internal/iot/";

char currentTime[6] = "00:00";
char meridian[3] = "AM";
char currentDate[11] = "01/01/2000";
char dayOfWeek[10] = "Monday";
char monthOfYear[10] = "January";

bool dateChanged = false;


int timeFontSize = 128;
int dateFontSize = 72;
int dayOfWeekFontSize = 36;

int dayOfWeekPosY = 18;
int datePosY = 72;
int timePosY = screenCenterY;

// ********** Possible Customizations Start ***********

int otherAppTopicCount = 0;
char otherAppTopic[10][25];
void (*otherAppMessageHandler[10])(char *topic, JsonDocument &doc);

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
bool checkGoodTime();
bool getNewTime();

void drawTime();
void app_loop();
void app_setup();

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
    ofr.setDrawer(tft);
    ofr.loadFont(Roboto, sizeof(Roboto));
    ofr.setFontColor(TFT_WHITE, TFT_BLACK);
    ofr.setFontSize(32);
    ofr.setAlignment(Align::MiddleCenter);

    drawString(appName, tft.width() / 2, tft.height() / 2, 56);

    char showText[100];
    if (appInstanceID < 0)
    {
        sprintf(showText, "Configuring...");
    }
    else
    {
        sprintf(showText, "Name: %s", friendlyName);
    }
    drawString(showText, tft.width() / 2, tft.height() / 2 + 40, 24);

    sprintf(showText, "Device ID: %i", appInstanceID);
    drawString(showText, tft.width() / 2, tft.height() - 20, 18);

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

    drawTime();

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
        preferences.getString("FriendlyName", friendlyName, 100); // 100 is the max length of the string
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
    preferences.putString("FriendlyName", friendlyName);
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

bool checkGoodTime()
{
    String oldMethodName = methodName;
    methodName = "checkGoodTime()";
    Log.verboseln("Entering...");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Log.errorln("Failed to obtain time");
        return false;
    }

    if (timeinfo.tm_year < (2020 - 1900))
    {
        Log.errorln("Failed to obtain time");
        return false;
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return true;
}

bool getNewTime()
{
    String oldMethodName = methodName;
    methodName = "getNewTime()";
    Log.verboseln("Entering...");

    bool isNewTime = false;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Log.errorln("Failed to obtain time");
        Log.verboseln("Exiting...");
        methodName = oldMethodName;
        return false;
    }

    char newTime[6] = "00:00";
    strftime(newTime, 20, "%I:%M", &timeinfo);
    strftime(meridian, 3, "%p", &timeinfo);

    if (strcmp(newTime, currentTime) != 0)
    {
        strcpy(currentTime, newTime);
        Log.infoln("Time is now %s %s", currentTime, meridian);

        isNewTime = true;
    }

    char newDate[6] = "01/01";
    strftime(newDate, 11, "%m/%d", &timeinfo);
    if (strcmp(newDate, currentDate) != 0)
    {
        strcpy(currentDate, newDate);
        strftime(dayOfWeek, 10, "%A", &timeinfo);
        strftime(monthOfYear, 10, "%B", &timeinfo);
        Log.infoln("Date is now %s", currentDate);
        dateChanged = true;
    }
    else
    {
        dateChanged = false;
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return isNewTime;
}

void drawDate()
{
    String oldMethodName = methodName;
    methodName = "drawDate()";
    Log.verboseln("Entering...");

    tft.fillRect(0, 0, tft.width(), 36, TFT_BLACK);
    drawString(dayOfWeek, screenCenterX, dayOfWeekPosY, dayOfWeekFontSize);

    tft.fillRect(0, 36, tft.width(), 72, TFT_BLACK);
    drawString(currentDate, screenCenterX, datePosY, dateFontSize);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void drawTime()
{
    String oldMethodName = methodName;
    methodName = "drawTime()";
    Log.verboseln("Entering...");

    tft.fillRect(0, middleCenterY - 64, tft.width(), 128, TFT_BLACK);
    drawString(currentTime, screenCenterX, middleCenterY, timeFontSize);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

IRAM_ATTR void interruptService()
{
}

void app_setup()
{
    String oldMethodName = methodName;
    methodName = "app_setup()";
    Log.verboseln("Entering...");

    // Add some custom code here
    initAppStrings();

    // Configure Hardware
    Log.infoln("Configuring hardware.");
    // pinMode(DOORBELL_PIN, INPUT);
    // attachInterrupt(digitalPinToInterrupt(DOORBELL_PIN), doorbellPressed, FALLING);

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void app_loop()
{
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

        if (isFirstDraw && isGoodTime)
        {
            isFirstDraw = false;
            clearScreen();
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
#endif // APP_FUNCTIONS_H