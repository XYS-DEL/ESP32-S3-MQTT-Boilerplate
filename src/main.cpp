#include <Arduino.h>
#include "config.h"
#include "iot_core.h"
#include "hal_sensor.h"
#include "sys_ota.h"

extern String clientId;

struct SensorPayload {
    float temperature;  // CPU温度
    uint32_t freeHeap;  // 剩余内存
    uint32_t timestamp; // 采集时间戳
};

TaskHandle_t Task_Net_Handle = NULL;
TaskHandle_t Task_HW_Handle = NULL;
QueueHandle_t dataQueue = NULL; // 两个核心之间传递数据的传送带

// ==========================================
// 2. 硬件核任务 (Core 1)
// ==========================================
void Task_Hardware(void *pvParameters) {
    Serial.printf("硬件任务运行在核心: %d\n", xPortGetCoreID());

    for (;;) {
        SensorPayload myData;
        
        // 装入最新的硬件数据
        myData.temperature = readInternalTemp();
        myData.freeHeap = ESP.getFreeHeap();
        myData.timestamp = millis();

        // 放进队列
        if (dataQueue != NULL) {
            // portMAX_DELAY: 如果传送带满了，就死等，直到有空位
            xQueueSend(dataQueue, &myData, portMAX_DELAY);
        }

        // 硬件核的特权：紧急本地控制（即使 Wi-Fi 断了也能报警）
        if (myData.temperature > 70.0) {
            Serial.println("警告：温度过高！");
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // 每 10 秒采集打包一次
    }
}

// ==========================================
// 3. 网络核任务 (Core 0) 
// ==========================================
void Task_Network(void *pvParameters) {
    Serial.printf("网络任务运行在核心: %d\n", xPortGetCoreID());

    setupIoTSystem(); 
    setupOTA(clientId);

    for (;;) {
        loopIoTSystem(); // 维持 MQTT 心跳
        loopOTA();       // 监听 OTA 升级呼叫

        SensorPayload receivedData;

        if (xQueueReceive(dataQueue, &receivedData, 0) == pdPASS) {
            publishStatusWithData(receivedData.temperature, receivedData.freeHeap, receivedData.timestamp);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // 给网络协议栈留点喘息时间，防止看门狗反应不过来
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); 

    initInternalTempSensor();

    dataQueue = xQueueCreate(5, sizeof(SensorPayload));

    if (dataQueue != NULL) {
        // 参数依次为：任务函数名, 任务代号, 分配的内存栈大小, 参数, 优先级, 句柄, 核心编号(0或1)
        xTaskCreatePinnedToCore(Task_Network, "NetTask", 8192, NULL, 1, &Task_Net_Handle, 0); 
        xTaskCreatePinnedToCore(Task_Hardware, "HWTask", 4096, NULL, 1, &Task_HW_Handle, 1);
    }

    // 彻底交给 FreeRTOS 调度
    vTaskDelete(NULL); 
}

void loop() {
    // 留空
}