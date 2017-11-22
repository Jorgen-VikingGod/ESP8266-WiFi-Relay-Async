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
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>                  //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>            //https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFSEditor.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"                  //https://github.com/bblanchon/ArduinoJson

#include "helper.h"

// declare and initial upload file container and web server
AsyncWebServer server(80);
AsyncEventSource events("/events");
DNSServer dns;

struct sRelay {
  uint8_t pin;
  uint8_t type;
  uint8_t state;
  sRelay(uint8_t relayPin, uint8_t relayType = HIGH, uint8_t relayState = LOW) {
    pin = relayPin;
    type = relayType;
    state = relayState;
  }
};

// declare and initial list of default relay settings
sRelay relay[8] = {sRelay(D5), sRelay(D6), sRelay(D7), sRelay(D8), sRelay(D3), sRelay(D4), sRelay(D2), sRelay(D1)};

bool shouldReboot = false;
bool inAPMode = false;

const char* hostName = "wifi-relay";
const char* http_username = "admin";
const char* http_password = "admin";


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
  pinMode(relay[3].pin, OUTPUT);
  pinMode(relay[4].pin, OUTPUT);
  pinMode(relay[5].pin, OUTPUT);
  pinMode(relay[6].pin, OUTPUT);
  pinMode(relay[7].pin, OUTPUT);

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
    DEBUG_PRINTLN(F(""));
  }

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
  // get relay state: GET http://wifi-relay.local/relay?id=0
  // set relay state: GET http://wifi-relay.local/relay?id=0&value=0 or 1
  server.on("/relay", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint8_t idx = request->getParam("id")->value().toInt() -1;
    if (request->hasParam("value")) {
      uint8_t value = request->getParam("value")->value().toInt();
      setRelay(idx, value);
      request->redirect("/");
    } else {
      sendRelay(idx, request);
    }
  });
  // set toggle state: POST http://wifi-relay.local/toggle with value=0 or 1
  server.on("/toggle", HTTP_POST, [](AsyncWebServerRequest *request) {
    String data = request->getParam("plain")->value();
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(data);
    String cmd = json["cmd"];
    String id = json["id"];
    String value = json["value"];
    if (cmd == "relay") {
      uint8_t idx = id.substring(5).toInt() -1;
      setRelay(idx, value.toInt());
      sendRelay(idx, request);
    }
  });
  // Simple Firmware Update Handler
  server.on("/settings/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("[ UPDT ] Firmware update started: %s\n", filename.c_str());
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("[ UPDT ] Firmware update finished: %uB\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // get status: GET http://wifi-relay.local/settings/status
  server.on("/settings/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendStatus(request);
  });

  // scan wifi networks: GET http://wifi-relay.local/settings/scanwifi
  server.on("/settings/scanwifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["command"] = "ssidlist";
    int networksFound = WiFi.scanComplete();
    if(networksFound == -2){
      WiFi.scanNetworks(true);
      root["error"] = "again";
      root.printTo(*response);
      request->send(response);
    } else if(networksFound) {
      JsonArray& scan = root.createNestedArray("list");
      for (int i = 0; i < networksFound; ++i) {
        JsonObject& item = scan.createNestedObject();
        // Print SSID for each network found
        item["ssid"] = WiFi.SSID(i);
        item["bssid"] = WiFi.BSSIDstr(i);
        item["rssi"] = WiFi.RSSI(i);
        item["channel"] = WiFi.channel(i);
        item["enctype"] = WiFi.encryptionType(i);
        item["hidden"] = WiFi.isHidden(i)?true:false;
      }
      root.printTo(*response);
      request->send(response);
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
  });

  // get config: GET http://wifi-relay.local/settings/configfile
  server.on("/settings/configfile", HTTP_GET, [](AsyncWebServerRequest *request) {
    WiFi.scanNetworks(true);
    //Send config.json content as json text
    request->send(SPIFFS, "/config.json", "text/json");
  });
  
  // define static links
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
  server.serveStatic("/settings", SPIFFS, "/settings.htm").setDefaultFile("settings.htm");
  server.serveStatic("/fonts", SPIFFS, "/fonts").setCacheControl("max-age=86400");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/css", SPIFFS, "/css").setCacheControl("max-age=86400");
  server.serveStatic("/images", SPIFFS, "/images").setCacheControl("max-age=86400");
  // handle file not found
  server.onNotFound([](AsyncWebServerRequest *request) {
    DEBUG_PRINT(F("NOT_FOUND: "));
    if (request->method() == HTTP_GET)
      DEBUG_PRINT(F("GET"));
    else if (request->method() == HTTP_POST)
      DEBUG_PRINT(F("POST"));
    else if (request->method() == HTTP_DELETE)
      DEBUG_PRINT(F("DELETE"));
    else if (request->method() == HTTP_PUT)
      DEBUG_PRINT(F("PUT"));
    else if (request->method() == HTTP_PATCH)
      DEBUG_PRINT(F("PATCH"));
    else if (request->method() == HTTP_HEAD)
      DEBUG_PRINT(F("HEAD"));
    else if (request->method() == HTTP_OPTIONS)
      DEBUG_PRINT(F("OPTIONS"));
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
    
    if (request->url() == "/settings/configfile") {
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject((const char*)data); 
      if (!root.success()) {
        DEBUG_PRINTLN(F("[WARN] Couldn't parse json object from request body"));
      } else {
        File f = SPIFFS.open("/config.json", "w+");
        if (f) {
          root.prettyPrintTo(f);
          f.close();
          delay(3000);
          //reset and try again, or maybe put it to deep sleep
          ESP.reset();
          delay(5000);
        }
      }
    }
  });
  // start webserver
  server.begin();
  DEBUG_PRINT(F("HTTP server started"));
  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);

    // Try to load configuration file so we can connect to an Wi-Fi Access Point
  // Do not worry if no config file is present, we fall back to Access Point mode and device can be easily configured
  if (!loadConfiguration()) {
    fallbacktoAPMode();
  }
}

// main loop
void loop()
{
  // check for a new update and restart
  if (shouldReboot) {
    DEBUG_PRINTLN(F("[UPDT] Rebooting..."));
    delay(100);
    ESP.restart();
  }
  ArduinoOTA.handle();
}

String printIP(IPAddress adress) {
  return (String)adress[0] + "." + (String)adress[1] + "." + (String)adress[2] + "." + (String)adress[3];
}

// Fallback to AP Mode, so we can connect to ESP if there is no Internet connection
void fallbacktoAPMode() {
  inAPMode = true;
  WiFi.mode(WIFI_AP);
  DEBUG_PRINT(F("[INFO] Configuring access point... "));
  DEBUG_PRINTLN(WiFi.softAP("wifi-relay") ? "Ready" : "Failed!");
  // Access Point IP
  IPAddress myIP = WiFi.softAPIP();
  DEBUG_PRINT(F("[INFO] AP IP address: "));
  DEBUG_PRINTLN(myIP);
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("settings.htm").setAuthentication("admin", "admin");
}

bool loadConfiguration() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    DEBUG_PRINTLN(F("[WARN] Failed to open config file"));
    return false;
  }
  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (!json.success()) {
    DEBUG_PRINTLN(F("[WARN] Failed to parse config file"));
    return false;
  }
  DEBUG_PRINTLN(F("[INFO] Config file found"));
  if (_debug) {
    json.prettyPrintTo(Serial);
  }
  DEBUG_PRINTLN("");
  const char * hstname = json["hostname"];
  const char * bssidmac = json["bssid"];
  byte bssid[6];
  parseBytes(bssidmac, ':', bssid, 6, 16);

  // Set Hostname.
  WiFi.hostname(hstname);

  // Start mDNS service so we can connect to http://wifi-relay.local (if Bonjour installed on Windows or Avahi on Linux)
  if (!MDNS.begin(hstname)) {
    DEBUG_PRINTLN(F("Error setting up MDNS responder!"));
  }
  // Add Web Server service to mDNS
  MDNS.addService("http", "tcp", 80);

  relay[0].type = json["relay1"]["type"];
  relay[0].pin = json["relay1"]["pin"];
  relay[1].type = json["relay2"]["type"];
  relay[1].pin = json["relay2"]["pin"];
  relay[2].type = json["relay3"]["type"];
  relay[2].pin = json["relay3"]["pin"];
  relay[3].type = json["relay4"]["type"];
  relay[3].pin = json["relay4"]["pin"];
  relay[4].type = json["relay5"]["type"];
  relay[4].pin = json["relay5"]["pin"];
  relay[5].type = json["relay6"]["type"];
  relay[5].pin = json["relay6"]["pin"];
  relay[6].type = json["relay7"]["type"];
  relay[6].pin = json["relay7"]["pin"];
  relay[7].type = json["relay8"]["type"];
  relay[7].pin = json["relay8"]["pin"];
  
  pinMode(relay[0].pin, OUTPUT);
  pinMode(relay[1].pin, OUTPUT);
  pinMode(relay[2].pin, OUTPUT);
  pinMode(relay[3].pin, OUTPUT);
  pinMode(relay[4].pin, OUTPUT);
  pinMode(relay[5].pin, OUTPUT);
  pinMode(relay[6].pin, OUTPUT);
  pinMode(relay[7].pin, OUTPUT);
  loadSettings();

  const char * ssid = json["ssid"];
  const char * password = json["wifipwd"];
  int wmode = json["wifimode"];
  const char * adminpass = json["adminpwd"];

  // Serve confidential files in /auth/ folder with a Basic HTTP authentication
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm").setAuthentication("admin", adminpass);

  if (wmode == 1) {
    inAPMode = true;
    DEBUG_PRINTLN(F("[INFO] WiFi-Relay is running in AP Mode "));
    WiFi.mode(WIFI_AP);
    DEBUG_PRINT(F("[INFO] Configuring access point... "));
    DEBUG_PRINTLN(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");
    // Access Point IP
    IPAddress myIP = WiFi.softAPIP();
    DEBUG_PRINT(F("[INFO] AP IP address: "));
    DEBUG_PRINTLN(myIP);
    DEBUG_PRINT(F("[INFO] AP SSID: "));
    DEBUG_PRINTLN(ssid);
    return true;
  }
  else if (!connectSTA(ssid, password, bssid)) {
    return false;
  }
  return true;
}

// Try to connect WiFi
bool connectSTA(const char* ssid, const char* password, byte bssid[6]) {
  WiFi.mode(WIFI_STA);
  // First connect to a wifi network
  WiFi.begin(ssid, password, 0, bssid);
  // Inform user we are trying to connect
  DEBUG_PRINT(F("[INFO] Trying to connect WiFi: "));
  DEBUG_PRINT(ssid);
  // We try it for 20 seconds and give up on if we can't connect
  unsigned long now = millis();
  uint8_t timeout = 20; // define when to time out in seconds
  // Wait until we connect or 20 seconds pass
  do {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(500);
    DEBUG_PRINT(F("."));
  }
  while (millis() - now < timeout * 1000);
  // We now out of the while loop, either time is out or we connected. check what happened
  if (WiFi.status() == WL_CONNECTED) { // Assume time is out first and check
    DEBUG_PRINTLN("");
    DEBUG_PRINT(F("[INFO] Client IP address: ")); // Great, we connected, inform
    DEBUG_PRINTLN(WiFi.localIP());
    return true;
  }
  else { // We couln't connect, time is out, inform
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN(F("[WARN] Couldn't connect in time"));
    return false;
  }
}

// load stored settings from EEPROM
void loadSettings()
{
  // load relay states
  relay[0].state = EEPROM.read(0);
  relay[1].state = EEPROM.read(1);
  relay[2].state = EEPROM.read(2);
  relay[3].state = EEPROM.read(3);
  relay[4].state = EEPROM.read(4);
  relay[5].state = EEPROM.read(5);
  relay[6].state = EEPROM.read(6);
  relay[7].state = EEPROM.read(7);
  // set relay states
  digitalWrite(relay[0].pin, relay[0].state);
  digitalWrite(relay[1].pin, relay[1].state);
  digitalWrite(relay[2].pin, relay[2].state);
  digitalWrite(relay[3].pin, relay[3].state);
  digitalWrite(relay[4].pin, relay[4].state);
  digitalWrite(relay[5].pin, relay[5].state);
  digitalWrite(relay[6].pin, relay[6].state);
  digitalWrite(relay[7].pin, relay[7].state);
}

// send all relay states back to website
void sendAll(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/json");
  // try to open config.json file
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println(F("[WARN] Failed to open config file"));
  }
  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (!json.success()) {
    Serial.println(F("[WARN] Failed to parse config file"));
  }
  DynamicJsonBuffer jsonBuffer2;
  JsonObject &root = jsonBuffer2.createObject();
  // create relay json objects
  JsonObject& relay1 = jsonBuffer2.createObject();
  relay1["name"] = json["relay1"]["name"];
  relay1["state"] = relay[0].state;
  root.set("relay1", relay1);
  JsonObject& relay2 = jsonBuffer2.createObject();
  relay2["name"] = json["relay2"]["name"];
  relay2["state"] = relay[1].state;
  root.set("relay2", relay2);
  JsonObject& relay3 = jsonBuffer2.createObject();
  relay3["name"] = json["relay3"]["name"];
  relay3["state"] = relay[2].state;
  root.set("relay3", relay3);
  JsonObject& relay4 = jsonBuffer2.createObject();
  relay4["name"] = json["relay4"]["name"];
  relay4["state"] = relay[3].state;
  root.set("relay4", relay4);
  JsonObject& relay5 = jsonBuffer2.createObject();
  relay5["name"] = json["relay5"]["name"];
  relay5["state"] = relay[4].state;
  root.set("relay5", relay5);
  JsonObject& relay6 = jsonBuffer2.createObject();
  relay6["name"] = json["relay6"]["name"];
  relay6["state"] = relay[5].state;
  root.set("relay6", relay6);
  JsonObject& relay7 = jsonBuffer2.createObject();
  relay7["name"] = json["relay7"]["name"];
  relay7["state"] = relay[6].state;
  root.set("relay7", relay7);
  JsonObject& relay8 = jsonBuffer2.createObject();
  relay8["name"] = json["relay8"]["name"];
  relay8["state"] = relay[7].state;
  root.set("relay8", relay8);
  root.printTo(*response);
  request->send(response);
}

// send relay state back to website (idx = index of relay)
void sendRelay(uint8_t idx, AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["relay"+String(idx+1)] = relay[idx].state;
  root.printTo(*response);
  request->send(response);
}

// set current relay state and store it in EEPROM
void setRelay(uint8_t idx, uint8_t value)
{
  uint8_t lowValue  = (relay[idx].type == 0 ? HIGH : LOW);
  uint8_t highValue = (relay[idx].type == 0 ? LOW  : HIGH);
  relay[idx].state = (value == 0 ? LOW : HIGH);
  digitalWrite(relay[idx].pin, (relay[idx].state == LOW ? lowValue : highValue));
  EEPROM.write(idx, relay[idx].state);
  EEPROM.commit();
}

// send all stats back to website
void sendStatus(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/json");
  struct ip_info info;
  FSInfo fsinfo;
  if (!SPIFFS.info(fsinfo)) {
    Serial.print(F("[ WARN ] Error getting info on SPIFFS"));
  }
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["command"] = "status";
  root["heap"] = ESP.getFreeHeap();
  root["chipid"] = String(ESP.getChipId(), HEX);
  root["cpu"] = ESP.getCpuFreqMHz();
  root["availsize"] = ESP.getFreeSketchSpace();
  root["availspiffs"] = fsinfo.totalBytes - fsinfo.usedBytes;
  root["spiffssize"] = fsinfo.totalBytes;
  if (inAPMode) {
    wifi_get_ip_info(SOFTAP_IF, &info);
    struct softap_config conf;
    wifi_softap_get_config(&conf);
    root["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
    root["dns"] = printIP(WiFi.softAPIP());
    root["mac"] = WiFi.softAPmacAddress();
  }
  else {
    wifi_get_ip_info(STATION_IF, &info);
    struct station_config conf;
    wifi_station_get_config(&conf);
    root["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
    root["dns"] = printIP(WiFi.dnsIP());
    root["mac"] = WiFi.macAddress();
  }
  IPAddress ipaddr = IPAddress(info.ip.addr);
  IPAddress gwaddr = IPAddress(info.gw.addr);
  IPAddress nmaddr = IPAddress(info.netmask.addr);
  root["ip"] = printIP(ipaddr);
  root["gateway"] = printIP(gwaddr);
  root["netmask"] = printIP(nmaddr);
  root.printTo(*response);
  request->send(response);
}
