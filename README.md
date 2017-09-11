# ESP8266-WiFi-Relay-Async
simple sketch for ESP8266 of using ESPAsyncWebServer to switch relays on GPIO pins, serves a simple website with toggle buttons for each relay and uses AsyncWiFiManager to configure WiFi network.

## Features
* Data is encoded as JSON object
* get and set relay states also by simple GET requests
* Bootstrap and jQuery for beautiful Web Pages for both Mobile and Desktop Screens
* ESPAsyncWebServer Library to use syncronous communication
* ESPAsyncWiFiManager Library to configure WiFi network by Access Point
* ArduinoOTA updates
* MDNS support

### What You Will Need 
### Hardware
* An ESP8266 module or development board like WeMos or NodeMcu with at least 32Mbit Flash (equals to 4MBytes)
* 3 Relay Modules

### Software
#### Building From Source
Please install Arduino IDE if you didn't already, then add ESP8266 Core (Beware! Install Git Version) on top of it. Additional Library download links are listed below:
* [Arduino IDE](http://www.arduino.cc) - The development IDE (tested with 1.8.3)
* [ESP8266 Core for Arduino IDE](https://github.com/esp8266/Arduino) - ESP8266 Core
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Asynchrone WebServer with WebSocket Plug-in
* [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) - Mandatory for ESPAsyncWebServer
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson) - JSON Library for Arduino IDE
* [ESPAsyncWiFiManager](https://github.com/alanswx/ESPAsyncWiFiManager) - Async port of [WiFiManager](https://github.com/tzapu/WiFiManager)

You also need to upload web files to your ESP with ESP8266FS Uploader.
* [ESP8266FS Uploader](https://github.com/esp8266/arduino-esp8266fs-plugin) - Arduino ESP8266 filesystem uploader

Unlisted libraries are part of ESP8266 Core for Arduino IDE, so you don't need to download them.

### Steps
* First, flash firmware by Arduino IDE or with your favourite flash tool
* Flash webfiles data to SPIFFS either using ESP8266FS Uploader tool or with your favourite flash tool 
* (optional) Fire up your serial monitor to get informed
* Power on your ESP8266
* Search for Wireless Network "wifi-relay" and connect to it (default password is **adminadmin**)
* Normally your system will open a page; otherwise open your browser and type either "http://192.168.4.1" or "http://wifi-relay.local" (.local needs Bonjour installed on your computer) on address bar.
* Scan for available networks and fill in your password and press save.
* ESP8266 will reboot and connects to your network.
* Check your new IP address from serial monitor or DHCP network settings and connect to your ESP8299 again. (You can also use  "http://wifi-relay.local").
* Toggle relay 1, relay 2 and relay 3 buttons on or off and enjoy your WiFi relay device.
* Congratulations, everything went well, if you encounter any issue feel free to ask help on GitHub.
