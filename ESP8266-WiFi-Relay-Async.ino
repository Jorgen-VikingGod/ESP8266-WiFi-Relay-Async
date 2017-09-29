/*
 * ESP8266-WiFi-Relay-Async.ino
 * ----------------------------------------------------------------------------
 * simple sketch of using ESP8266WebServer to switch relays on GPIO pins
 * it serves a simple website with toggle buttons for each relay
 * additional it handles up to 3 servo motors
 * ----------------------------------------------------------------------------
 * Source:     https://github.com/Jorgen-VikingGod/ESP8266-WiFi-Relay-Async
 * Copyright:  Copyright (c) 2017 Juergen Skrotzky
 * Author:     Juergen Skrotzky <JorgenVikingGod@gmail.com>
 * License:    MIT License
 * Created on: 29.Sep. 2017
 * ----------------------------------------------------------------------------
 */

extern "C" {
#include "user_interface.h"
}

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Hash.h>
#include <Servo.h>
#include <FS.h>
#include "ArduinoJson.h"                  //https://github.com/bblanchon/ArduinoJson
#include "webserver.h"

Servo servo1;
Servo servo2;
Servo servo3;

struct sMask {
  Servo *servo;
  int pin;
  uint8_t open;
  uint8_t close;
  uint8_t state;
  uint8_t sweep;
  sMask(Servo *pServo = nullptr, int maskPin = -1, uint8_t maskOpen = 20, uint8_t maskClose = 160, uint8_t maskState = 1, uint8_t maskSweep = 0) {
    servo = pServo;
    pin = maskPin;
    open = maskOpen;
    close = maskClose;
    state = maskState;
    sweep = maskSweep;
  }
};
volatile sMask mask[3] = {sMask(&servo1, D1, 20, 90, 1, 0), sMask(&servo2, D2, 90, 20, 1, 0), sMask(&servo3, D3, 45, 135, 1, 0)};

struct sRelay {
  int pin;
  uint8_t type;
  uint8_t state;
  sRelay(int relayPin = -1, uint8_t relayType = 1, uint8_t relayState = LOW) {
    pin = relayPin;
    type = relayType;
    state = relayState;
  }
};
volatile sRelay relay[5] = {sRelay(D4,1,LOW), sRelay(D5,1,LOW), sRelay(D6,1,LOW), sRelay(D7,1,LOW), sRelay(D8,1,LOW)};

ESP8266WiFiMulti WiFiMulti;

void setup() {
  if (_debug) {
    Serial.begin(115200);
  }
  DEBUG_PRINT("\n");
  // use EEPROM
  EEPROM.begin(512);
  // use SPIFFS
  SPIFFS.begin();
  // handle requests
  server.on("/list", HTTP_GET, handleFileList);
  server.on("/edit", HTTP_GET, handleGetEditor);
  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  server.on("/edit", HTTP_POST, []() { server.send(200, "text/plain", ""); }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/all", HTTP_GET, handleGetAll);
  server.on("/relay", HTTP_GET, handleGetRelay);
  server.on("/mask", HTTP_GET, handleGetMask);
  server.on("/toggle", HTTP_POST, handlePostToggle);
  server.on("/settings/status", HTTP_GET, handleGetStatus);
  server.on("/settings/configfile", HTTP_GET, handleGetConfigfile);
  server.on("/settings/configfile", HTTP_POST, handlePostConfigfile);
  server.begin();
  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  // load config.json and connect to WiFI
  if (!loadConfiguration()) {
    // if no configuration found,
    // try to connect hard coded multi APs
    connectSTA(nullptr, nullptr, "wifi-relay");
    // initialize IO pins for relays
    pinMode(relay[0].pin, OUTPUT);
    pinMode(relay[1].pin, OUTPUT);
    pinMode(relay[2].pin, OUTPUT);
    pinMode(relay[3].pin, OUTPUT);
    pinMode(relay[4].pin, OUTPUT);
    // attach servo pins
    mask[0].servo->attach(mask[0].pin);
    mask[1].servo->attach(mask[1].pin);
    mask[2].servo->attach(mask[2].pin);
  }
  // load last states from EEPROM
  loadSettings();
}

void loop() {
  // switch servo motors if needed
  for (uint8_t idx = 0; idx < 3; idx++) {
    if (mask[idx].sweep && mask[idx].servo) {
      uint8_t servoValue = (mask[idx].state ? mask[idx].open : mask[idx].close);
      DEBUG_PRINTF("servo%d.write(%d)\n", idx + 1, servoValue);
      if (!mask[idx].servo->attached()) {
        mask[idx].servo->attach(mask[idx].pin);
      }
      mask[idx].servo->write(servoValue);
      delay(500);
      if (mask[idx].servo->attached()) {
        mask[idx].servo->detach();
      }
      mask[idx].sweep = 0;
    }
    yield();
  }
  // handle http clients
  server.handleClient();
}

// load stored settings from EEPROM
void loadSettings() {
  // load relay states
  relay[0].state = EEPROM.read(0);
  relay[1].state = EEPROM.read(1);
  relay[2].state = EEPROM.read(2);
  relay[3].state = EEPROM.read(3);
  relay[4].state = EEPROM.read(4);
  // load mask states
  mask[0].state = EEPROM.read(5);
  mask[1].state = EEPROM.read(6);
  mask[2].state = EEPROM.read(7);
  DEBUG_PRINTF("loadSettings: relay1: %d, relay2: %d, relay3: %d, relay4: %d, relay5: %d, mask1: %d, mask2: %d, mask3: %d\n", relay[0].state, relay[1].state, relay[2].state, relay[3].state, relay[4].state, mask[0].state, mask[1].state, mask[2].state);
  // set relay states
  digitalWrite(relay[0].pin, relay[0].state);
  digitalWrite(relay[1].pin, relay[1].state);
  digitalWrite(relay[2].pin, relay[2].state);
  digitalWrite(relay[3].pin, relay[3].state);
  digitalWrite(relay[4].pin, relay[4].state);
  // set sweep flags to force servo motors to drive on correct state (open or close)
  mask[0].sweep = 1;
  mask[1].sweep = 1;
  mask[2].sweep = 1;
}

// load config.json file and try to connect
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
  // get relay settings
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
  // initialize relay pins
  pinMode(relay[0].pin, OUTPUT);
  pinMode(relay[1].pin, OUTPUT);
  pinMode(relay[2].pin, OUTPUT);
  pinMode(relay[3].pin, OUTPUT);
  pinMode(relay[4].pin, OUTPUT);
  // get mask settings
  mask[0].open = json["mask1"]["open"];
  mask[0].close = json["mask1"]["close"];
  mask[0].pin = json["mask1"]["pin"];
  mask[1].open = json["mask2"]["open"];
  mask[1].close = json["mask2"]["close"];
  mask[1].pin = json["mask2"]["pin"];
  mask[2].open = json["mask3"]["open"];
  mask[2].close = json["mask3"]["close"];
  mask[2].pin = json["mask3"]["pin"];
  // attach servo motor pins
  mask[0].servo->attach(mask[0].pin);
  mask[1].servo->attach(mask[1].pin);
  mask[2].servo->attach(mask[2].pin);
  // get stored wifi settings
  const char * ssid = json["ssid"];
  const char * password = json["wifipwd"];
  const char * hostname = json["hostname"];
  // try to connect with stored settings and hard coded ones
  if (!connectSTA(ssid, password, hostname)) {
    return false;
  }
  return true;
}

// Try to connect WiFi
bool connectSTA(const char* ssid, const char* password, const char * hostname) {
  WiFi.mode(WIFI_STA);
  // add here your hard coded backfall wifi ssid and password
  //WiFiMulti.addAP("<YOUR-SSID-1>", "<YOUR-WIFI-PASS-1>");
  //WiFiMulti.addAP("<YOUR-SSID-2>", "<YOUR-WIFI-PASS-2>");
  if (ssid && password) {
    WiFiMulti.addAP(ssid, password);
  }
  // We try it for 30 seconds and give up on if we can't connect
  unsigned long now = millis();
  uint8_t timeout = 30; // define when to time out in seconds
  DEBUG_PRINT(F("[INFO] Trying to connect WiFi: "));
  while (WiFiMulti.run() != WL_CONNECTED) {
    if (millis() - now < timeout * 1000) {
      delay(200);
      DEBUG_PRINT(F("."));
    } else {
      DEBUG_PRINTLN("");
      DEBUG_PRINTLN(F("[WARN] Couldn't connect in time"));
      return false;
    }
  }
  if (hostname) {
    if (MDNS.begin(hostname)) {
      DEBUG_PRINTLN(F("MDNS responder started"));
    }
  }
  DEBUG_PRINTLN("");
  DEBUG_PRINT(F("[INFO] Client IP address: ")); // Great, we connected, inform
  DEBUG_PRINTLN(WiFi.localIP());
  return true;
}

// send ESP8266 status
void sendStatus() {
  DEBUG_PRINTLN("sendStatus()");
  struct ip_info info;
  FSInfo fsinfo;
  if (!SPIFFS.info(fsinfo)) {
    DEBUG_PRINTLN(F("[ WARN ] Error getting info on SPIFFS"));
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
  wifi_get_ip_info(STATION_IF, &info);
  struct station_config conf;
  wifi_station_get_config(&conf);
  root["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
  root["dns"] = printIP(WiFi.dnsIP());
  root["mac"] = WiFi.macAddress();
  IPAddress ipaddr = IPAddress(info.ip.addr);
  IPAddress gwaddr = IPAddress(info.gw.addr);
  IPAddress nmaddr = IPAddress(info.netmask.addr);
  root["ip"] = printIP(ipaddr);
  root["gateway"] = printIP(gwaddr);
  root["netmask"] = printIP(nmaddr);
  String json;
  root.printTo(json);
  DEBUG_PRINTLN(json);
  server.setContentLength(root.measureLength());
  server.send(200, "application/json", json);
}

// send whole configfile and status
void sendConfigfile() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    DEBUG_PRINTLN(F("[WARN] Failed to open config file"));
  }
  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(buf.get());
  if (!root.success()) {
    DEBUG_PRINTLN(F("[WARN] Failed to parse config file"));
  }
  DEBUG_PRINTLN(F("[INFO] Config file found"));
  struct ip_info info;
  FSInfo fsinfo;
  if (!SPIFFS.info(fsinfo)) {
    DEBUG_PRINTLN(F("[ WARN ] Error getting info on SPIFFS"));
  }
  root["heap"] = ESP.getFreeHeap();
  root["chipid"] = String(ESP.getChipId(), HEX);
  root["cpu"] = ESP.getCpuFreqMHz();
  root["availsize"] = ESP.getFreeSketchSpace();
  root["availspiffs"] = fsinfo.totalBytes - fsinfo.usedBytes;
  root["spiffssize"] = fsinfo.totalBytes;
  wifi_get_ip_info(STATION_IF, &info);
  struct station_config conf;
  wifi_station_get_config(&conf);
  root["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
  root["dns"] = printIP(WiFi.dnsIP());
  root["mac"] = WiFi.macAddress();
  IPAddress ipaddr = IPAddress(info.ip.addr);
  IPAddress gwaddr = IPAddress(info.gw.addr);
  IPAddress nmaddr = IPAddress(info.netmask.addr);
  root["ip"] = printIP(ipaddr);
  root["gateway"] = printIP(gwaddr);
  root["netmask"] = printIP(nmaddr);
  String json;
  root.printTo(json);
  DEBUG_PRINTLN(json);
  server.setContentLength(root.measureLength());
  server.send(200, "application/json", json);
}

// send all states
void sendAll() {
  DEBUG_PRINTLN("sendAll()");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["relay1"] = relay[0].state;
  root["relay2"] = relay[1].state;
  root["relay3"] = relay[2].state;
  root["relay4"] = relay[3].state;
  root["relay5"] = relay[4].state;
  root["mask1"] = mask[0].state;
  root["mask2"] = mask[1].state;
  root["mask3"] = mask[2].state;
  String json;
  root.printTo(json);
  DEBUG_PRINTLN(json);
  server.setContentLength(root.measureLength());
  server.send(200, "application/json", json);
}

// set current relay state and store it in EEPROM
void setRelay(uint8_t idx, uint8_t value) {
  DEBUG_PRINTF("setRelay(%d, %d)\n", idx, value);
  relay[idx].state = (value == 0 ? LOW : HIGH);
  digitalWrite(relay[idx].pin, relay[idx].state);
  EEPROM.write(idx, relay[idx].state);
  EEPROM.commit();
}

// send relay state back to website (idx = index of relay)
void sendRelay(uint8_t idx) {
  DEBUG_PRINTF("sendRelay(%d)\n", idx);
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["relay"+String(idx+1)] = relay[idx].state;
  String json;
  root.printTo(json);
  DEBUG_PRINTLN(json);
  server.setContentLength(root.measureLength());
  server.send(200, "application/json", json);
}

// set current mask state and store it in EEPROM
void setMask(uint8_t idx, uint8_t value) {
  DEBUG_PRINTF("setMask(%d, %d)\n", idx, value);
  mask[idx].state = value;
  mask[idx].sweep = 1;
  EEPROM.write(5 + idx, mask[idx].state);
  EEPROM.commit();
}

// send mask state back to website (idx = index of mask)
void sendMask(uint8_t idx) {
  DEBUG_PRINTF("sendMask(%d)\n", idx);
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["mask"+String(idx + 1)] = mask[idx].state;
  String json;
  root.printTo(json);
  DEBUG_PRINTLN(json);
  server.setContentLength(root.measureLength());
  server.send(200, "application/json", json);
}
