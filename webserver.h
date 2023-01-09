/******************************************************************
 * 
******************************************************************/

#ifndef webserver_h
#define webserver_h

//###################################################################
#include "config_html.h"
#include "directory_html.h"

// #include <AsyncWebServer_WT32_ETH01.h>
#include <Update.h>  // https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/src/Update.h

AsyncWebServer webserver(80);

//-------------------------------------------------------------------
// specify some content-type mapping, so the resources get delivered with the
// right content type and are displayed correctly in the browser
char contentTypes[][2][32] = {
  {".htm", "text/html"},
  {".html", "text/html"},
  {".txt", "text/plain"},
  {".css", "text/css"},
  {".js", "application/javascript"},
  {".png", "image/png"},
  {".gif", "image/gif"},
  {".jpg", "image/jpeg"},
  {".ico", "image/x-icon"},
  {".xml", "text/xml"},
  {".pdf", "application/x-pdf"},
  {".zip", "application/x-zip"},
  {".gz", "application/x-gzip"},
  {"", ""}
};

//-------------------------------------------------------------------
// Replaces placeholder with button section in your web page 
// the section character, %, is used to identify the key word(s)
String keyValueLookup(const String& var){
  //Serial.println(var);
  if     (var == "TEST1"){ return "result1"; }
  else if(var == "TEST2"){ return "result2"; }
  else if(var == "CompDate") return __DATE__;
  else if(var == "CompTime") return __TIME__;
  else if(var == "CompFile") return __FILE__;
  else if(var == "TotalHeap") { char tmpData[8]; itoa(ESP.getHeapSize(), tmpData, 10); return tmpData; }
  else if(var == "FreeHeap") { char tmpData[8]; itoa(ESP.getFreeHeap(), tmpData, 10); return tmpData; }
  else if(var == "millis") { char tmpData[8]; itoa(millis(), tmpData, 10); return tmpData; }
  else if(var == "restartCount") { char tmpData[8]; itoa(restartCount, tmpData, 10); return tmpData; }
  else if(var == "i2cAddresses") { char tmpData[320]; 
                                   strcpy(tmpData, "no I2C addresses found");
                                   for (int i=0; i<sizeof(i2cAddresses); i++){
                                    if (i2cAddresses[i] == 0) { break; }
                                    char tmpAddr[8];
                                    if (i == 0) {
                                      // for the 1st entry overwrite the warning message
                                      sprintf(tmpAddr, "x%02X", i2cAddresses[i]);
                                      strcpy(tmpData, tmpAddr);
                                    } else {
                                      // append next address
                                      strcat(tmpData, ", ");
                                      sprintf(tmpAddr, "x%02X", i2cAddresses[i]);
                                      strcat(tmpData, tmpAddr);
                                    }
                                   }
                                   return tmpData; }
  
  else   { return "PARAMTER NOT FOUND"; }
  return "";
}

//-------------------------------------------------------------------
// display the tradional root page, "index.html" if it exists, or a  suggestion page if not
void onRoot_handler(AsyncWebServerRequest * request ){
  if (SPIFFS.exists("/index.html")) {
    // redirect to the index.html file
    AsyncWebServerResponse *response = request->beginResponse(301, "text/html", "<!DOCTYPE HTML><html><head><meta http-equiv='refresh' content='0'; url='/index.html'/></head><body><h2>redirect to target page</h2><a href='/index.html'>Home</a></body></html>");
    response->addHeader("Location", "/index.html");   // https://en.wikipedia.org/wiki/HTTP_302#:~:text=The%20HTTP%20response%20status%20code,way%20of%20performing%20URL%20redirection.
    request->redirect("/index.html");
    request->send(response);    
  } else {
    // file hasn't been uploaded yet so offer hints and links to do so
    request->send(404, "text/html", "<html><head><title>File Not Found</title><head><body><h2>There is no index.html file</h2><hr>Upload a file with the <a href='/directory'>directory's upload</a> function</br> or try the <a href='/config'>config page</a></body></html>");
  }
}

//-------------------------------------------------------------------
void onConfig_handler(AsyncWebServerRequest * request ){
  request->send_P(200, "text/html", index_html, keyValueLookup);
}

//-------------------------------------------------------------------
void handleDirectory(AsyncWebServerRequest * request) {
  request->send_P(200, "text/html", directory_html);
}

//-------------------------------------------------------------------
void onFileHandler(AsyncWebServerRequest * request) {
  // passing URI query parameters not yet implented
  if (isVerbose) {Serial.print(F("onFileHandler: Executing on core: ")); Serial.print(xPortGetCoreID()); Serial.print(F(";  priority: ")); Serial.println(uxTaskPriorityGet(NULL));}
  std::string fileName = request->url().c_str();
  int params = request->params();

  if (isVerbose) {
    Serial.println(F("Requested file:"));
    for (int i = 0; i < params; i++) 
    {
      AsyncWebParameter* p = request->getParam(i);
      
      if (p->isPost()) 
      {
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } 
      else 
      {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    Serial.print("Page requested: "); Serial.println(fileName.c_str());   // https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/README.md#common-variables
  }

  if (!request->method() == HTTP_GET){   // will alwasy be a get because the "server.on" definition is only looking for HTTP_GET
    request->send(404, "text/plain", "Cannot continue.  Expected method 'GET' was not used.");
    return;
  }

  if (!SPIFFS.exists(fileName.c_str())) {
    request->send(404, "text/plain", "File does not exist.");
    return;
  }
    
  File file = SPIFFS.open(fileName.c_str());

  // Content-Type is guessed using the definition of the contentTypes-table defined above
  int cTypeIdx = 0;
  do {
    if(fileName.rfind(contentTypes[cTypeIdx][0])!=std::string::npos) {   // https://cplusplus.com/reference/string/string/rfind/
        if (isVerbose) {Serial.println(contentTypes[cTypeIdx][1]);}
      break;
    }
    cTypeIdx+=1;
  } while(strlen(contentTypes[cTypeIdx][0])>0);

  // https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/README.md#respond-with-content-coming-from-a-file-and-extra-headers
  AsyncWebServerResponse *response;
  if (cTypeIdx == 0 || cTypeIdx ==1 || cTypeIdx == 2) {
    // it is safe to replace keywords in text-type files
    response = request->beginResponse(SPIFFS, fileName.c_str(), String(), false, keyValueLookup);
  } else {
    // do NOT modify non-text files
    response = request->beginResponse(SPIFFS, fileName.c_str());
  }
  response->addHeader("Content-Type", contentTypes[cTypeIdx][1]);
  request->send(response);

}

//-------------------------------------------------------------------
void onUpdatePostProcesser(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    //Upload handler chunks in data
    bool isOTA = false;
    if(request->hasParam("isOTA", true) && request->getParam("isOTA", true)->value()) { isOTA = true; }
    if (!index) {
      if (isVerbose) {
        // show the parameters
        Serial.println(F("Parameters in the WebServerRequest:"));
        int params = request->params();
        for (int i = 0; i < params; i++) 
        {
          AsyncWebParameter* p = request->getParam(i);
          
          if (p->isPost()) 
          {
            Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          } 
          else 
          {
            Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        }
      }
          if(!request->hasParam("MD5", true)) {
              return request->send(400, "text/plain", "MD5 parameter missing");
          }

          if(!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
              return request->send(400, "text/plain", "MD5 parameter invalid");
          }

      if (isOTA) {
        int cmd = (filename == "filesystem") ? U_SPIFFS : U_FLASH;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { // Start with max available size
          Update.printError(Serial);
          return request->send(400, "text/plain", "OTA could not begin");
        }
      } else {
        // actions for a typical file
        request->_tempFile = SPIFFS.open("/" + filename, "w");
        if (!request->_tempFile) { 
          return request->send(400, "text/plain", "File upload could not begin");
        }
      }          
    }

    // Write chunked data to the free sketch space
    if(len){
      if (isOTA) {
        if (Update.write(data, len) != len) {
            return request->send(400, "text/plain", "OTA could not begin");
        }
      } else {
        // actions for a typical file
        if (request->_tempFile.write(data, len) != len) {
            return request->send(400, "text/plain", "File upload FAILED to write all data");
        }
      }
    }
        
    if (final) { // if the final flag is set then this is the last frame of data
      if (isOTA) {
        if (!Update.end(true)) { //true to set the size to the current progress
            Update.printError(Serial);
            return request->send(400, "text/plain", "Could not end OTA");
        }
      } else {
        // actions for a typical file
        request->_tempFile.close();
      }
    }else{
        return;
    }
}

//-------------------------------------------------------------------
void onUpdatePostHandler(AsyncWebServerRequest * request){
    // the request handler is triggered after the upload has finished... 
    // create the response, add header, and send response
    bool isOTA = false;
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
//    AsyncWebServerResponse *response = request->beginResponse((Update.hasError())?500:200, (Update.hasError())?"text/plain":"text/html", (Update.hasError())?"FAIL":"<!DOCTYPE HTML><html><head><meta http-equiv='refresh' content='3'; url='/directory'/></head><body><h2>SUCCESS</h2><a href='/'>Home</a></body></html>");
//    response->addHeader("Connection", "close");
//    response->addHeader("Access-Control-Allow-Origin", "*");
//    request->redirect("/directory");
    request->send(response);

    if(request->hasParam("isOTA", true) && request->getParam("isOTA", true)->value()) { isOTA = true; }
    if (isOTA) {
      espRestart();
    } else {
      // no follow actions for a typical file
    }
}

//-------------------------------------------------------------------
// very risky! there is no validation or authorization.
// ANYONE can remove files from the server
void handleRemoveFile(AsyncWebServerRequest * request) {
  if (request->method() == HTTP_GET){
    if(request->hasParam("filename")){
      AsyncWebParameter* p = request->getParam("filename");
//Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      char fName[33];   // max SPIFFs name is 32
      strcpy (fName,"/");  // all SPIFFs files must start with /
      strcat  (fName,p->value().c_str());
      if (SPIFFS.exists(fName)) {
        SPIFFS.remove(fName);
        request->send(200, "text/html", "<html><head><title>Remove File</title><meta http-equiv='refresh' content='1; url=/directory'/><head><body>Removed file</body></html>");
      } else {
        request->send(404, "text/plain", "Cannot remove.  File does not exist.");
      }
    } else {
      request->send(404, "text/plain", "Cannot remove.  Parameter 'filename' is missing.");
    }
  } else {
    request->send(404, "text/plain", "Cannot remove.  Expected method 'GET' was not used.");
  }
}

//-------------------------------------------------------------------
void handleReboot(AsyncWebServerRequest * request) {
  request->send(200, "text/html", "<!DOCTYPE html><html><head><title>REBOOT</title><meta http-equiv='refresh' content='15; url=/'/></head><h1>REBOOTING</h1></body></html>");
  espRestart();
}

//-------------------------------------------------------------------
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/html", "<html><head><title>File Not Found</title><head><body>File not found<hr>Try the <a href='/'>default page</a></body></html>");
}

//###################################################################

void webserver_setup(){
  websocket.onEvent(onWsEvent);
  webserver.addHandler(&websocket);

  webserver.on("/config", HTTP_GET, &onConfig_handler );
  webserver.on("/", HTTP_GET, &onRoot_handler );
  webserver.on("/directory", HTTP_GET, &handleDirectory);
  webserver.on("/remove", HTTP_GET, &handleRemoveFile);
  webserver.on("/reboot", HTTP_GET, &handleReboot);
  webserver.on("/update", HTTP_POST, &onUpdatePostHandler, &onUpdatePostProcesser );   // For OTA
  webserver.on("/*", HTTP_GET, &onFileHandler );

  webserver.onNotFound(&notFound);

  webserver.begin();

}

//-------------------------------------------------------------------
void webserver_loop(){

}

#endif      //webserver_h
