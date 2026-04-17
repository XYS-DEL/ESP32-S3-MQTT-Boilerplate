#ifndef SYS_OTA_H
#define SYS_OTA_H
#include <Arduino.h>

void setupOTA(const String& hostname);
void loopOTA();

#endif