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

class Servo {
public:
  Servo(int minPulseWidth = 500, int maxPulseWidth = 2400) : m_minPulseWidth(minPulseWidth), m_maxPulseWidth(maxPulseWidth) {}
  void setup(int servoPin, int minPulseWidth = 500, int maxPulseWidth = 2400) {
    m_servoPin = servoPin;
    m_minPulseWidth = minPulseWidth;
    m_maxPulseWidth = maxPulseWidth;
    // configure pin mode
    pinMode(m_servoPin, OUTPUT);
    //setup pwm for 50Hz (20ms)
    analogWriteFreq(50);
    //setup pwm for range of 20000 or 1 equals 1uS
    analogWriteRange(20000);
  }
  void setAngleRange(int minAngle = 0, int maxAngle = 180) {
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;
  }
  void sweep(int angle) {
    // limit angle to configured range between min and max
    int angleBound = constrain(angle, m_minAngle, m_maxAngle);
    // convert angle to correct pulse width in us
    int anglePulseWidth = map(angleBound, 0, 180, m_minPulseWidth, m_maxPulseWidth);
    // write out the frequence 
    analogWrite(m_servoPin, anglePulseWidth);
  }
private:
  int m_servoPin;
  int m_minPulseWidth;
  int m_maxPulseWidth;
  int m_minAngle;
  int m_maxAngle;
};

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
