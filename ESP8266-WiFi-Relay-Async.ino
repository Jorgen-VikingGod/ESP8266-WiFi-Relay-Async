/*
 * ESP8266-WiFi-Relay-Async.ino
 * ----------------------------------------------------------------------------
 * simple sketch of using ESPAsyncWebServer to switch relays on GPIO pins
 * it serves a simple website with toggle buttons for each relay
 * and uses AsyncWiFiManager to configure WiFi network
 * ----------------------------------------------------------------------------
 * Source:     https://github.com/Jorgen-VikingGod/ESP8266-WiFi-Relay-Async
 * Copyright:  Copyright (c) 2017 Juergen Skrotzky
 * Author:     Juergen Skrotzky <JorgenVikingGod@gmail.com>
 * License:    MIT License
 * Created on: 11.Sep. 2017
 * ----------------------------------------------------------------------------
 */

extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>                  //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>            //https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFSEditor.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESPAsyncWiFiManager.h>          //https://github.com/alanswx/ESPAsyncWiFiManager
#include "AsyncJson.h"
#include "ArduinoJson.h"                  //https://github.com/bblanchon/ArduinoJson

#include "helper.h"

AsyncWebServer server(80);
AsyncEventSource events("/events");
DNSServer dns;

const char* hostName = "wifi-relay";
const char* http_username = "admin";
const char* http_password = "admin";

// define pins for relay 1 to 3
#define RELAY1 D5
#define RELAY2 D7
#define RELAY3 D8

struct sRelay {
  int pin;
  uint8_t state;
  sRelay(int relayPin = -1, uint8_t relayState = LOW) {
    pin = relayPin;
    state = relayState;
  }
};
sRelay relay[3] = {sRelay(RELAY1,LOW), sRelay(RELAY2,LOW), sRelay(RELAY3,LOW)};

ESP8266WiFiMulti WiFiMulti;

// forward declaration
void loadSettings();
void sendAll(AsyncWebServerRequest *request);
void sendRelay(uint8_t idx, AsyncWebServerRequest *request);
void setRelay(uint8_t idx, uint8_t value);

void setup()
{
  if (_debug) {
    Serial.begin(115200);
  }
  DEBUG_PRINT("\n");
  Serial.setDebugOutput(true);

  // configure pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relay[0].pin, OUTPUT);
  pinMode(relay[1].pin, OUTPUT);
  pinMode(relay[2].pin, OUTPUT);

  // load settings
  EEPROM.begin(512);
  loadSettings();

  DEBUG_PRINTLN("");
  DEBUG_PRINT(F("Heap: ")); DEBUG_PRINTLN(system_get_free_heap_size());
  DEBUG_PRINT(F("Boot Vers: ")); DEBUG_PRINTLN(system_get_boot_version());
  DEBUG_PRINT(F("CPU: ")); DEBUG_PRINTLN(system_get_cpu_freq());
  DEBUG_PRINT(F("SDK: ")); DEBUG_PRINTLN(system_get_sdk_version());
  DEBUG_PRINT(F("Chip ID: ")); DEBUG_PRINTLN(system_get_chip_id());
  DEBUG_PRINT(F("Flash ID: ")); DEBUG_PRINTLN(spi_flash_get_id());
  DEBUG_PRINT(F("Flash Size: ")); DEBUG_PRINTLN(ESP.getFlashChipRealSize());
  DEBUG_PRINT(F("Vcc: ")); DEBUG_PRINTLN(ESP.getVcc());
  DEBUG_PRINTLN("");

  // initialize filesystem
  SPIFFS.begin();
  if (_debug) {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DEBUG_PRINTF("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }
    DEBUG_PRINTF("\n");
  }

  // configure WiFiManager
  AsyncWiFiManager wifiManager(&server,&dns);
  //reset settings - for testing
  //wifiManager.resetSettings();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here "wifi-relay"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(hostName, "adminadmin")) {
    DEBUG_PRINTLN("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  
  DEBUG_PRINT("");
  DEBUG_PRINT("Connected! IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  // configure OTA Updates and send OTA events to the browser
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();

  // begin MDNS to access device with http://wifi-relay.local
  // you need bonjour:
  // for windows:  https://support.apple.com/kb/DL999?locale=en_US&viewlocale=de_DE
  MDNS.begin(hostName);
  
  DEBUG_PRINT("Open http://");
  DEBUG_PRINT(hostName);
  DEBUG_PRINT(".local/edit to see the file browser");
  
  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);
  server.addHandler(new SPIFFSEditor(http_username,http_password));

  // handle requests
  // get all relay states: GET http://wifi-relay.local/all
  server.on("/all", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendAll(request);
  });
  // get relay1 state: GET http://wifi-relay.local/relay_1
  // set relay1 state: GET http://wifi-relay.local/relay_1?value=0 or 1
  server.on("/relay_1", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      AsyncWebParameter* pPayload = request->getParam("value");
      String value = pPayload->value();
      setRelay(0, value.toInt());
      request->redirect("/");
    } else {
      sendRelay(0, request);
    }
  });
  // set relay1 state: POST http://wifi-relay.local/relay_1 with value=0 or 1
  server.on("/relay_1", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncWebParameter* pPayload = request->getParam("value");
    String value = pPayload->value();
    setRelay(0, value.toInt());
    sendRelay(0, request);
  });
  // get relay2 state: GET http://wifi-relay.local/relay_2
  // set relay2 state: GET http://wifi-relay.local/relay_2?value=0 or 1
  server.on("/relay_2", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      AsyncWebParameter* pPayload = request->getParam("value");
      String value = pPayload->value();
      setRelay(1, value.toInt());
      request->redirect("/");
    } else {
      sendRelay(1, request);
    }
  });
  // set relay2 state: POST http://wifi-relay.local/relay_2 with value=0 or 1
  server.on("/relay_2", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncWebParameter* pPayload = request->getParam("value");
    String value = pPayload->value();
    setRelay(1, value.toInt());
    sendRelay(1, request);
  });
  // get relay3 state: GET http://wifi-relay.local/relay_3
  // set relay3 state: GET http://wifi-relay.local/relay_3?value=0 or 1
  server.on("/relay_3", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      AsyncWebParameter* pPayload = request->getParam("value");
      String value = pPayload->value();
      setRelay(2, value.toInt());
      request->redirect("/");
    } else {
      sendRelay(2, request);
    }
  });
  // set relay3 state: POST http://wifi-relay.local/relay_3 with value=0 or 1
  server.on("/relay_3", HTTP_POST, [](AsyncWebServerRequest *request) {
    AsyncWebParameter* pPayload = request->getParam("value");
    String value = pPayload->value();
    setRelay(2, value.toInt());
    sendRelay(2, request);
  });
  // define static links
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
  server.serveStatic("/fonts", SPIFFS, "/fonts").setCacheControl("max-age=86400");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/css", SPIFFS, "/css").setCacheControl("max-age=86400");
  server.serveStatic("/images", SPIFFS, "/images").setCacheControl("max-age=86400");
  // handle file not found
  server.onNotFound([](AsyncWebServerRequest *request) {
    DEBUG_PRINTF("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      DEBUG_PRINTF("GET");
    else if (request->method() == HTTP_POST)
      DEBUG_PRINTF("POST");
    else if (request->method() == HTTP_DELETE)
      DEBUG_PRINTF("DELETE");
    else if (request->method() == HTTP_PUT)
      DEBUG_PRINTF("PUT");
    else if (request->method() == HTTP_PATCH)
      DEBUG_PRINTF("PATCH");
    else if (request->method() == HTTP_HEAD)
      DEBUG_PRINTF("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      DEBUG_PRINTF("OPTIONS");
    else
      DEBUG_PRINTF("UNKNOWN");
    DEBUG_PRINTF(" http://%s%s\n", request->host().c_str(), request->url().c_str());
    if (request->contentLength()) {
      DEBUG_PRINTF("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      DEBUG_PRINTF("_CONTENT_LENGTH: %u\n", request->contentLength());
    }
    int headers = request->headers();
    int i;
    for (i=0;i<headers;i++) {
      AsyncWebHeader* h = request->getHeader(i);
      DEBUG_PRINTF("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
    int params = request->params();
    for (i=0;i<params;i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isFile()) {
        DEBUG_PRINTF("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if (p->isPost()) {
        DEBUG_PRINTF("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        DEBUG_PRINTF("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(404);
  });
  // file upload
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index)
      DEBUG_PRINTF("UploadStart: %s\n", filename.c_str());
    DEBUG_PRINTF("%s", (const char*)data);
    if (final)
      DEBUG_PRINTF("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!index)
      DEBUG_PRINTF("BodyStart: %u\n", total);
    DEBUG_PRINTF("%s", (const char*)data);
    if (index + len == total)
      DEBUG_PRINTF("BodyEnd: %u\n", total);
  });
  // start webserver
  server.begin();
  DEBUG_PRINT("HTTP server started");
  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
}

// main loop
void loop()
{
  ArduinoOTA.handle();
  // add custom code here
  // t.b.d.
}

// load stored settings from EEPROM
void loadSettings()
{
  // load relay states
  relay[0].state = EEPROM.read(0);
  relay[1].state = EEPROM.read(1);
  relay[2].state = EEPROM.read(2);
  // set relay states
  digitalWrite(relay[0].pin, relay[0].state);
  digitalWrite(relay[1].pin, relay[1].state);
  digitalWrite(relay[2].pin, relay[2].state);
}

// send all relay states back to website
void sendAll(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["relay_1"] = relay[0].state;
  root["relay_2"] = relay[1].state;
  root["relay_3"] = relay[2].state;
  root.printTo(*response);
  request->send(response);
}

// send relay state back to website (idx = index of relay)
void sendRelay(uint8_t idx, AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["relay_"+String(idx+1)] = relay[idx].state;
  root.printTo(*response);
  request->send(response);
}

// set current relay state and store it in EEPROM
void setRelay(uint8_t idx, uint8_t value)
{
  relay[idx].state = (value == 0 ? LOW : HIGH);
  digitalWrite(relay[idx].pin, relay[idx].state);
  EEPROM.write(idx, relay[idx].state);
  EEPROM.commit();
}

