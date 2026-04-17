#include "hal_sensor.h"
#include <Arduino.h>

void initInternalTempSensor() {
    Serial.println("内部温度传感器已就绪");
}

float readInternalTemp() {
    // 调用官方底层库函数直接读取温度
    return temperatureRead();
}