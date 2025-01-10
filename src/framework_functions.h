#ifndef FRAMEWORK_FUNCTIONS_H
#define FRAMEWORK_FUNCTIONS_H

#include "framework.h"

void initFS();
void connectToWifi();
void connectToMqtt();
void resetWifiFailCount(TimerHandle_t xTimer);
void doUpdateFirmware(char *fileName);
int getlatestFirmware(char *fileName);
int webGet(String req, String &res);
void checkFWUpdate();
void onWifiConnect(const WiFiEvent_t &event);
void onWifiDisconnect(const WiFiEvent_t &event);
void WiFiEvent(WiFiEvent_t event);
void mqttPublishWill();
void mqttPublishID();
bool isNumeric(char *str);
bool checkMessageForAppSecret(JsonDocument &doc);
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void logMACAddress(uint8_t baseMac[6]);
void setAppInstanceID();
void setupFramework();

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

    Log.infoln("Connecting to MQTT broker...");
    mqttClient.connect();

    methodName = oldMethodName;
}

void resetWifiFailCount(TimerHandle_t xTimer)
{
    String oldMethodName = methodName;
    methodName = "resetWifiFailCount(TimerHandle_t xTimer)";
    Log.verboseln("Entering...");

    (void)xTimer;

    wifiFailCount = 0;

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void doUpdateFirmware(char *fileName)
{
    String oldMethodName = methodName;
    methodName = "doUpdateFirmware(char *fileName)";
    Log.verboseln("Entering...");

    String fsFileName = "/" + String(fileName);

    File file = SPIFFS.open(fsFileName);

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
        // TODO: publish a message that the update failed

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
        // TODO: publish a message that the update failed
        return;
    }

    file.close();

    Log.infoln("Reset in 2 seconds...");
    delay(2000);

    ESP.restart();
}

int getlatestFirmware(char *fileName)
{
    String oldMethodName = methodName;
    methodName = "int getlatestFirmware(char *fileName)";

    int result = -1;
    int httpCode = -1;

    WiFiClient client;
    HTTPClient http;
    String payload;

    String fsFileName = "/" + String(fileName);

    File f = SPIFFS.open(fsFileName, FILE_WRITE);
    if (f)
    {
        Log.verboseln("[HTTP] begin...");

        // int connRes = client.connect(IPAddress(192,168,0,12), 5000);
        // Log.verboseln("Connected: %d", connRes);

        // if (http.begin(client, req))
        String url = String("/firmware/") + fileName;

        if (http.begin(client, HTTP_SERVER, HTTP_PORT, url))
        { // HTTP

            Log.verboseln("[HTTP] GET...");
            // start connection and send HTTP header
            int httpCode = http.GET();
            result = httpCode;
            if (httpCode > 0)
            {
                if (httpCode == HTTP_CODE_OK)
                {
                    WiFiClient *stream = http.getStreamPtr();
                    while (stream->available())
                    {
                        char c = stream->read();
                        f.print(c);
                    }
                }
            }
            else
            {
                Log.verboseln("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            f.close();
            http.end();
        }
        else
        {
            Log.warningln("[HTTP] Unable to connect");
        }
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return httpCode;
}

int webGet(String req, String &res)
{
    String oldMethodName = methodName;
    methodName = "webGet(String req, String &res)";

    int result = -1;

    Log.verboseln("Connecting to http://%s:%d%s", HTTP_SERVER, HTTP_PORT, req.c_str());

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

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
    return result;
}

void checkFWUpdate()
{
    String oldMethodName = methodName;
    methodName = "checkFWUpdate()";

    String fileList;
    String server_req;
    int latestFWImageIndex = appVersion;

    Log.infoln("Checking for FW updates.");
    int code = webGet(firmwareUrl, fileList);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileList);
    for (size_t i = 0; i < doc.size(); i++)
    {
        String fn = doc[i]["name"];
        if ((doc[i]["type"] == "file") && fn.startsWith(appName) && fn.endsWith(".bin"))
        {
            Log.verboseln("Got firmware file: %s", fn.c_str());
            String findex = fn.substring(strlen(appName) + 1, fn.length() - 4);
            int fidx = findex.toInt();
            Log.verboseln("Got firmware index: %d", fidx);
            latestFWImageIndex = max(latestFWImageIndex, fidx);
        }
    }

    if (latestFWImageIndex > appVersion)
    {
        Log.infoln("New firmware available: v%d", latestFWImageIndex);
        sprintf(latestFirmwareFileName, "%s_%d.bin", appName, latestFWImageIndex);
        Log.infoln("Downloading %s", latestFirmwareFileName);
        code = getlatestFirmware(latestFirmwareFileName);

        Log.infoln("Updating firmware...");
        doUpdateFirmware(latestFirmwareFileName);
    }
    else
    {
        Log.infoln("No new firmware available.");
    }

    Log.verboseln("Exiting...");
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

    if (isGoodTime = checkGoodTime())
        Log.infoln("Connected to NTP Server!");
    else
        Log.errorln("Failed to connect to NTP Server!");

    if (appInstanceID >= 0)
    {
        Log.infoln("Checking for FW updates...");
        checkFWUpdate();
    }

    connectToMqtt();

    ProcessWifiConnectTasks();

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
    ProcessWifiDisconnectTasks();
    Log.infoln("Disconnecting mqttReconnectTimer");
    xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    // mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    if (wifiFailCount == 0)
    {
        wifiFailCountTimer = xTimerCreate("wifiFailCountTimer", pdMS_TO_TICKS(10000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(resetWifiFailCount));
        xTimerStart(wifiFailCountTimer, pdMS_TO_TICKS(wifiFailCountTimeLimit * 1000));
    }
    wifiFailCount++;
    if (wifiFailCount > maxWifiFailCount)
    {
        Log.errorln("Too many WiFi failures. Rebooting.");
        esp_restart();
    }

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

    char onlineTopic[51];
    char payloadJson[100];
    sprintf(onlineTopic, "%s/online", appName);
    sprintf(payloadJson, "{ \"appInstanceID\" : \"%i\" , \"online\" : \"true\" }", appInstanceID);
    //sprintf(payloadJson, "{ \"appInstanceID\" : \"%i\" }", appInstanceID);

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

    char onlineTopic[51];
    char payloadJson[100];
    sprintf(onlineTopic, "%s/online", appName);
    sprintf(payloadJson, "{ \"appInstanceID\" : \"%i\" , \"online\" : \"false\" }", appInstanceID);
    Log.infoln("Published Last Will and Testament %s", onlineTopic);
    mqttClient.publish(onlineTopic, 1, true, payloadJson);

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

    if (appInstanceID > -1)
    {
        mqttPublishID();
        //mqttPublishWill();
        ProcessMqttConnectTasks();
    }
    else
    {
        // mqttClient.publish(idTopic, 1, false);
        Log.infoln("Don't have appInstanceID yet so nothing to publish to MQTT.");
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

    ProcessMqttDisconnectTasks();

    if (WiFi.isConnected())
    {
        xTimerStart(mqttReconnectTimer, 0);
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void logMQTTMessage(char *topic, int len, char *payload)
{
    char msg[len + 1];
    memcpy(msg, payload, len);
    msg[len] = 0;

    Log.verboseln("[%s] %s", topic, msg);
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
            int otherIndex = -1;
            // TODO: Load into JsonDocument to extract appInstanceID
            char msg[len + 1];
            memcpy(msg, payload, len);
            msg[len] = 0;

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, msg);

            // Test if parsing succeeds.
            if (!error)
            {
                if (doc["appInstanceID"].is<int>())
                {
                    otherIndex = doc["appInstanceID"];
                    Log.infoln("Got appInstanceID: %d", otherIndex);
                }
            }
            else
            {
                Log.errorln("deserializeJson() failed: %s", error.c_str());
            }

            if (otherIndex >= 0)
                maxOtherIndex = max(maxOtherIndex, otherIndex);
        }
    }

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

bool checkMessageForAppSecret(JsonDocument &doc)
{
    if (doc["appSecret"].is<const char *>())
    {
        if (doc["appSecret"] == appSecret)
        {
            Log.verboseln("Got appSecret");
            return true;
        }
        Log.verboseln("Got appSecret but it is wrong.");
    }
    else
        Log.verboseln("Did not get appSecret");
    return false;
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    String oldMethodName = methodName;
    methodName = "onMqttMessage()";
    Log.verboseln("Entering...");

    logMQTTMessage(topic, len, payload);

    char topics[10][25];
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

        Log.verboseln("Exiting...");
        methodName = oldMethodName;
        return;
    }

    Log.verboseln("Processing MQTT message...");
    if ((strcmp(topic, appName) == 0) && (topicCounter > 1))
    {
        // Handle all messages for our App (AppName is in topic #1)
        Log.infoln("Got our topic");

        // Check for appSecret
        if (isNumeric(topics[1]))
        {
            char msg[len + 1];
            memcpy(msg, payload, len);
            msg[len] = 0;

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, msg);

            // Test if parsing succeeds.
            if (!error)
            {
                if (checkMessageForAppSecret(doc))
                {
                    // This is a command because there is an AppInstanceID in topic #2
                    int cmdTargetID = atoi(topics[1]);
                    if ((cmdTargetID == appInstanceID) || (cmdTargetID == -1))
                    { // This command is for us (AppInstanceID == our appInstanceID or ALL appInstanceID)
                        appMessageHandler(topic, doc);
                    }
                    else
                    { // This command is for another instance
                    }
                }
                else
                {
                    Log.errorln("AppSecret not found in message!");
                }
            }
            else
            {
                Log.errorln("deserializeJson() failed: %s", error.c_str());
            }
        }
        else
        { // This is not a properly formatted command (might be a response)
            Log.infoln("Not a command. No appInstanceID in subtopic #2. Could be our response. Ignoring...");
            logMQTTMessage(topic, len, payload);
        }
    }
    else
    {
        for (int i = 0; i < otherAppTopicCount; i++)
        {
            if (strcmp(topics[i], "otherAppTopic") == 0)
            {
                // This is another App's message we are interested in
                Log.infoln("Got otherThing topic");

                char msg[len + 1];
                memcpy(msg, payload, len);
                msg[len] = 0;

                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, msg);

                // Test if parsing succeeds.
                if (!error)
                {
                    otherAppMessageHandler[i](topic, doc);
                }
                else
                {
                    Log.errorln("deserializeJson() failed: %s", error.c_str());
                }
            }
        }
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

void setupFramework()
{
    // Framework: Setting up logging
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
    // Framework region end

    Log.noticeln("Starting %s v%d...", appName, appVersion);

    // Framework: Setting up app framework
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

    initFS();

    setupDisplay();
    // Framework region end

    // This is connectivity setup code
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE,
                                      (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE,
                                      (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

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
        appInstanceIDWaitTimer = xTimerCreate("appInstanceIDWaitTimer", pdMS_TO_TICKS(10000),
                                              pdFALSE, (void *)0,
                                              reinterpret_cast<TimerCallbackFunction_t>(setAppInstanceID));
        xTimerStart(appInstanceIDWaitTimer, 0);
    }
}

#endif // FRAMEWORK_FUNCTIONS_H