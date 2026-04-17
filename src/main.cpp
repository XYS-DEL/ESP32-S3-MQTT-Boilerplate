// src/main.cpp
#include <Arduino.h>
#include "iot_core.h"
#include "config.h"
#include "hal_sensor.h"
#include "sys_ota.h"

extern String clientId;

void setup() {
  Serial.begin(115200);
  delay(1000); 

  initInternalTempSensor();
  setupIoTSystem();
  setupOTA(clientId);
}

void loop() {
  loopIoTSystem();
  loopOTA();
}