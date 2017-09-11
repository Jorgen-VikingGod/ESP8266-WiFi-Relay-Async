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

#endif // _HELPER_H
