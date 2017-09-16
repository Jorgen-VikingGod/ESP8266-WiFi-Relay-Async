
# Arduino libraries for ESP8266-WiFi-Relay-Async firmware

|  Name | Date  | Version  | Source  | Commit |
|---|---|---|---|---|
|  ESPAsyncWebServer |  Sep 11th 2017 | N/A  | [source](https://github.com/me-no-dev/ESPAsyncWebServer)  | [a7db995](https://github.com/me-no-dev/ESPAsyncWebServer/commit/a7db995633990220d12401345805c203a94eb1a2)  |
|  ESPAsyncTCP |  Sep 11th 2017 | N/A  | [source](https://github.com/me-no-dev/ESPAsyncTCP)  | [9b0cc37](https://github.com/me-no-dev/ESPAsyncTCP/commit/9b0cc37cd7bdf4b7d17a363141d2e988aa46652c)  |
|  ArduinoJson |  Aug 30th 2017 | N/A  | [source](https://github.com/bblanchon/ArduinoJson)  | [57defe0](https://github.com/bblanchon/ArduinoJson/commit/57defe00ee5a311637495fe23398e7d026ead64b)  |

# Installing ESP8266-WiFi-Relay-Async Arduino Libraries

## Using Git Sub Modules

```
$ git clone https://github.com/Jorgen-VikingGod/ESP8266-WiFi-Relay-Async
$ cd ESP8266-WiFi-Relay-Async
$ git submodule update
$ git submodule init
$ git submodule update
```

Using sub modules has the advantage of keeping the link between the origional lib repository while at the same time allowing us to specify the state of each library. We have implemented this to ensure when a user clones the ESP8266-WiFi-Relay-Async fimware and attempts to compile we can be sure that the libs used are **exactly** the same as the libs used when we compiled the firmware.

## Arduino IDE setup

**Tested with Arduino 1.8.3**

To tell Arduino IDE to use the libraries in `ESP8266-WiFi-Relay-Async/libraries` we need to set the **Arduino IDE Sketchbook location** to `*<localpath>*/ESP8266-WiFi-Relay-Async` then restart the Arduino IDE.

On compiling check that Arduino is used the correct library, turn on *preferences>Show verbose output during compilation* and see log message at the beginning of compilation showing lib path. You might need to remove any lib you have sharing the same name in your Arduino sketchbook folder.
