////////////////////////////////////////////////////////////////////
/// @file web_server.h
/// @brief Contains all functions related to the webserver
////////////////////////////////////////////////////////////////////

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "framework.h"

#include "webpages.h"

const char *cssFile = "/mvp.css";
const char *selectedCSS = mvp_min_css;

char logoPath[50];

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
bool checkUserWebAuth(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);
void configureWebServer();
String processor(const String &var);

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml)
{
    String returnText = "";
    Log.infoln("Listing files stored on LittleFS");
    File root = LittleFS.open("/");
    File foundfile = root.openNextFile();
    if (ishtml)
    {
        returnText += "<table><thead><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr></thead>";
    }
    while (foundfile)
    {
        if (ishtml)
        {
            if (foundfile.isDirectory())
            {
                returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td></td>";
                returnText += "<td></td>";
                returnText += "<td></td></tr>";
            }
            else
            {
                returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
                returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
                returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
            }
        }
        else
        {
            returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
        }
        foundfile = root.openNextFile();
    }
    if (ishtml)
    {
        returnText += "</table>";
    }
    root.close();
    foundfile.close();
    return returnText;
}

// parses and processes webpages
// if the webpage has %SOMETHING% or %SOMETHINGELSE% it will replace those strings with the ones defined
String processor(const String &var)
{
    if (var == "FIRMWARE")
    {
        return FIRMWARE_VERSION;
    }

    if (var == "FREESPACE")
    {
        return humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes()));
    }

    if (var == "USEDSPACE")
    {
        return humanReadableSize(LittleFS.usedBytes());
    }

    if (var == "TOTALSPACE")
    {
        return humanReadableSize(LittleFS.totalBytes());
    }

    if (var == "APP_NAME")
    {
        return String(appName);
    }

    if (var == "CSS_FILE")
    {
        return String(cssFile);
    }

    return "Undefined!";
}

void secureGetCSS(AsyncWebServerRequest *request)
{
    String logmessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();

    if (checkUserWebAuth(request))
    {
        logmessage += " Auth: Success";
        Log.infoln(logmessage.c_str());
    }
    else
    {
        logmessage += " Auth: Failed";
        Log.infoln(logmessage.c_str());
        return request->requestAuthentication();
    }
}

void secureGetResponse(AsyncWebServerRequest *request)
{
    String oldMethodName = methodName;
    methodName = "secureGetResponse(AsyncWebServerRequest *request)";
    Log.verboseln("Entering");

    String logmessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();
    bool bsuccess = false;

    if (checkUserWebAuth(request))
    {
        bsuccess = true;
        //logmessage += " Auth: Success";
        //Log.infoln(logmessage.c_str());
        const char *output;
        if (strcmp(request->url().c_str(), "/") == 0)
            request->send_P(200, "text/html", index_html, processor);
        else if (strcmp(request->url().c_str(), "/reboot") == 0)
        {
            request->send_P(200, "text/html", reboot_html, processor);
            shouldReboot = true;
        }
        else if (strcmp(request->url().c_str(), "/logout") == 0)
        {
            request->requestAuthentication();
            request->send(401);
        }
        else if (strcmp(request->url().c_str(), "/reboot") == 0)
        {
            request->send_P(200, "text/html", reboot_html, processor);
            shouldReboot = true;
        }
        else if (strcmp(request->url().c_str(), "/listfiles") == 0)
        {
            request->send(200, "text/plain", listFiles(true));
        }
        else if (strcmp(request->url().c_str(), cssFile) == 0)
        {
            request->send_P(200, "text/css", selectedCSS, processor);
        }
        else if (strcmp(request->url().c_str(), logoPath) == 0)
        {
            request->send(LittleFS, appIconFilename, "image/png");
        }
    }
    else
    {
        logmessage += " Auth: Failed";
        //Log.infoln(logmessage.c_str());
        return request->requestAuthentication();
    }

    Log.infoln("Client %s Requested: %s - Auth %s", request->client()->remoteIP().toString(), request->url(), bsuccess ? "Succeeded!" : "Failed!!!");

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void initWebServer()
{
    String oldMethodName = methodName;
    methodName = "initWebServer()";
    Log.verboseln("Entering");

    // configure web webServer
    Log.infoln("Initializing Web Server on Port 80");

    // if url isn't found
    webServer.onNotFound(notFound);

    // run handleUpload function when any file is uploaded
    webServer.onFileUpload(handleUpload);

    // visiting this page will cause you to be logged out
    webServer.on("/logout", HTTP_GET, secureGetResponse);

    // presents a "you are now logged out webpage
    webServer.on("/logged-out", HTTP_GET, secureGetResponse);

    webServer.on("/", HTTP_GET, secureGetResponse);


    webServer.on(cssFile, HTTP_GET, secureGetResponse);

    webServer.on("/reboot", HTTP_GET, secureGetResponse);

    webServer.on("/listfiles", HTTP_GET, secureGetResponse);

    sprintf(logoPath, "/images/%s.png", appName);
    Log.infoln("Setting logo path...");
    webServer.on(logoPath, HTTP_GET, secureGetResponse);

    webServer.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Log.infoln(logmessage.c_str());

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!LittleFS.exists(fileName))
        {
            logmessage += " ERROR: file does not exist";
            Log.infoln(logmessage.c_str());
            request->send(400, "text/plain", "ERROR: file does not exist");
        }
        else
        {
            logmessage += " file exists";
            Log.infoln(logmessage.c_str());
            if (strcmp(fileAction, "download") == 0)
            {
                logmessage += " downloaded";
                request->send(LittleFS, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            LittleFS.remove(fileName);
            request->send(200, "text/plain", "Deleted File: " + String(fileName));
          } else {
            logmessage += " ERROR: invalid action param supplied";
            request->send(400, "text/plain", "ERROR: invalid action param supplied");
          }
          Log.infoln(logmessage.c_str());
        }
      } else {
        request->send(400, "text/plain", "ERROR: name and action params required");
      }
    } else {
      logmessage += " Auth: Failed";
      Log.infoln(logmessage.c_str());
      return request->requestAuthentication();
    } });

    webServer.begin();

    Log.verboseln("Exiting...");
    methodName = oldMethodName;
}

void notFound(AsyncWebServerRequest *request)
{
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "Not Found";
    Log.infoln(logmessage.c_str());
    request->send(404, "text/plain", "Not found");
}

// used by webServer.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool checkUserWebAuth(AsyncWebServerRequest *request)
{
    bool isAuthenticated = false;

    if (request->authenticate(appName, appSecret))
    {
        Log.verboseln("is authenticated via username and password");
        isAuthenticated = true;
    }
    return isAuthenticated;
}

// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    String oldMethodName = methodName;
    methodName = "drawSplashScreen()";
    Log.verboseln("Entering");

    // make sure authenticated before allowing upload
    if (checkUserWebAuth(request))
    {
        String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        Log.infoln(logmessage.c_str());

        if (!index)
        {
            logmessage = "Upload Start: " + String(filename);
            // open the file on first call and store the file handle in the request object
            request->_tempFile = LittleFS.open("/" + filename, "w");
            Log.infoln(logmessage.c_str());
        }

        if (len)
        {
            // stream the incoming chunk to the opened file
            request->_tempFile.write(data, len);
            logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
            Log.infoln(logmessage.c_str());
        }

        if (final)
        {
            logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
            // close the file handle as the upload is now done
            request->_tempFile.close();
            Log.infoln(logmessage.c_str());
            request->redirect("/");
        }
    }
    else
    {
        Log.infoln("Auth: Failed");
        return request->requestAuthentication();
    }
}

#endif // WEB_SERVER_H