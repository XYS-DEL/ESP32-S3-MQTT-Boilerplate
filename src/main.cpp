// src/main.cpp
#include <Arduino.h>
#include "iot_core.h"

void setup() {
  Serial.begin(115200);
  delay(1000); 

  setupIoTSystem();
}

void loop() {
  loopIoTSystem();
}