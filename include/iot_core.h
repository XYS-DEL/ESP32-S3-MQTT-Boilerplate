// include/iot_core.h
#include <Arduino.h>
#include <stdint.h>
#ifndef IOT_CORE_H
#define IOT_CORE_H


void setupIoTSystem();
void loopIoTSystem();
void publishStatusWithData(float temp, uint32_t freeHeap, uint32_t timestamp);

#endif