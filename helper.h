/*
 * helper.h
 * ----------------------------------------------------------------------------
 * helper methods and defines
 * ----------------------------------------------------------------------------
 * Source:     https://github.com/Jorgen-VikingGod/ESP8266-WiFi-Relay-Async
 * Copyright:  Copyright (c) 2017 Juergen Skrotzky
 * Author:     Juergen Skrotzky <JorgenVikingGod@gmail.com>
 * License:    MIT License
 * Created on: 11.Sep. 2017
 * ----------------------------------------------------------------------------
 */

#ifndef _HELPER_H
#define _HELPER_H

/*
 * Debug mode
 */
#define _debug 1

template <typename... Args>
void DEBUG_PRINTF(const char *format, Args &&...args) {
  if (_debug) {
    Serial.printf(format, args...);
  }
}
template <typename Generic>
void DEBUG_PRINT(Generic text) {
  if (_debug) {
    Serial.print(text);    
  }
}
template <typename Generic, typename Format>
void DEBUG_PRINT(Generic text, Format format) {
  if (_debug) {
    Serial.print(text, format);
  }
}
template <typename Generic>
void DEBUG_PRINTLN(Generic text) {
  if (_debug) {
    Serial.println(text);    
  }
}
template <typename Generic, typename Format>
void DEBUG_PRINTLN(Generic text, Format format) {
  if (_debug) {
    Serial.println(text, format);
  }
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                              // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}

#endif // _HELPER_H
