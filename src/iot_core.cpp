// src/iot_core.cpp
#include <Arduino.h>
#include <stdint.h>
#include "iot_core.h"
#include "config.h"
#include "sys_ota.h"
#include "hal_sensor.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <LittleFS.h>

// --- 全局变量声明 ---
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEfjCCA2agAwIBAgIQD+Ayq4RNAzEGxQyOE8iwaDANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaWNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0yNDAxMTgwMDAwMDBaFw0zMTExMDkyMzU5NTlaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo4IBMDCC
ASwwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUTiJUIBiV5uNu5g/6+rkS7QYX
jzkwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUwDgYDVR0PAQH/BAQD
AgGGMHQGCCsGAQUFBwEBBGgwZjAjBggrBgEFBQcwAYYXaHR0cDovL29jc3AuZGln
aWNlcnQuY24wPwYIKwYBBQUHMAKGM2h0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNu
L0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNydDBABgNVHR8EOTA3MDWgM6Axhi9odHRw
Oi8vY3JsLmRpZ2ljZXJ0LmNuL0RpZ2lDZXJ0R2xvYmFsUm9vdENBLmNybDARBgNV
HSAECjAIMAYGBFUdIAAwDQYJKoZIhvcNAQELBQADggEBAHRBl3jN7+XHBUK0dZnu
hMdoNwD1nCROU3BTIh1TNzRI0bQ0m5+C/dCRzzlqoSAFHUlOi+OiDltWkXTzmQn6
Z8bH5PFBy5sYpc/8cNPoSzhyqcpvvEZvv/Ivc0Up+dzma7vBDJC9WrMRUUlSFSQp
kdXSmphDNkXJsgARmxzc18IN6LYMRiOWlY7RE2F900pPW60BvJHHNCX0bbSRj/Ql
bmVq8wuftBD++D+RS8K++ujpMjFBROyWfBX+woQDGsMazkmgulQdnZrdj476elOL
axRvrSgEorju1kJM7M65z2RUZrfzQYW/1rs8mRUXin6iEtad/Rv1ZI1WGYmWPyBm
pbo=
-----END CERTIFICATE-----
)EOF";

String clientId, topic_state, topic_command;
// 文件缓存位置与熔断值
const char* CACHE_FILE = "/data_cache.log";
const size_t MAX_CACHE_SIZE = 512 * 1024;

WiFiClientSecure espClient; 
PubSubClient client(espClient);

Adafruit_NeoPixel rgb(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// 状态机全局控制变量
unsigned long lastReconnectAttempt = 0;
int mqttRetryCount = 0; // 用于记录 MQTT 连续失败次数

// --- 内部私有函数声明 ---
void setupWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectToMQTT();
void saveToCache(const char* data);
void publishCachedData();

// ==========================================
// 对外暴露的 Setup 函数
// ==========================================
void setupIoTSystem() {
    rgb.begin();
    rgb.setPixelColor(0, rgb.Color(0, 0, 0));
    rgb.show();

    if(!LittleFS.begin(true)) { 
        Serial.println("LittleFS 挂载失败！");
        return;
    }
    Serial.println("LittleFS 本地文件系统挂载成功！");

    setupWiFi();

    String mac = WiFi.macAddress();
    mac.replace(":", ""); 

    clientId = "ESP32S3-" + mac;
    topic_state = "device/esp32s3/" + clientId + "/state";
    topic_command = "device/esp32s3/" + clientId + "/command";

    setupOTA(clientId);

    Serial.println("\n--- MQTT Topic ---");
    Serial.println("状态上报: " + topic_state);
    Serial.println("命令下发: " + topic_command);

    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqttCallback);

    // 协议栈保活与超时配置
    client.setKeepAlive(45); 
    client.setSocketTimeout(15);
}

// ==========================================
// 对外暴露的 Loop 函数
// ==========================================
void loopIoTSystem() {
    loopOTA();
    unsigned long now = millis();

    // 第一层守护：检查物理底层的 Wi-Fi 状态
    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            Serial.println("物理网络(Wi-Fi)已断开，正在尝试重新连接...");
            WiFi.reconnect(); 
        }
        return; 
    }

    // 第二层守护：Wi-Fi 正常，检查上层的 MQTT 状态
    if (!client.connected()) {
        if (now - lastReconnectAttempt > 5000) { 
            lastReconnectAttempt = now;
            mqttRetryCount++; // 失败次数累加
            
            Serial.print("Wi-Fi 正常，尝试重连 MQTT (第 ");
            Serial.print(mqttRetryCount);
            Serial.println(" 次)...");
            
            connectToMQTT(); 

            // 核心防御：连续 6 次连接失败，触发 lwIP 协议栈重置
            if (mqttRetryCount >= 6) {
                Serial.println("协议栈假死！执行网络重置...");
                
                WiFi.disconnect(true, true); // 彻底抹除底层状态
                delay(500); 
                WiFi.mode(WIFI_STA);
                WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // 重新发起 L2/L3 握手
                
                mqttRetryCount = 0; 
            }
        }
    } else {
        // 第三层：一切正常，清零计数器，维持心跳
        mqttRetryCount = 0;
        client.loop(); 
    }
}

// ==========================================
//  具体业务实现
// ==========================================

void setupWiFi() {
    int i = 0;
    WiFi.setSleep(false);
    WiFi.disconnect(true, true);
    delay(100);

    WiFi.mode(WIFI_STA); 
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print(" 正在连接 WiFi... ");

    // NTP 时间同步（TLS 前置条件）
    Serial.print(" 正在同步网络时间(NTP)...");
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov"); 

    time_t now = time(nullptr);
    while (now < 24 * 3600) { 
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(" 时间同步完成！");

    while(WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
        if(++i >= 15) { // 稍微延长等待时间，防止路由器响应慢
            Serial.println("连接超时！请检查 WiFi 状况！");
            return;
        }
    }

    Serial.println(" WiFi 连接成功！");
    Serial.print(" IP 地址为: ");
    Serial.println(WiFi.localIP());

    espClient.setCACert(root_ca);
    Serial.println("TLS/SSL 根证书已加载，准备进行加密握手...");
}

void connectToMQTT() {
    Serial.print("尝试连接 MQTT Broker...");
    
    String willPayload = "{\"status\": \"offline\"}";
    
    boolean success = client.connect(
        clientId.c_str(),    
        NULL,                
        NULL,                
        topic_state.c_str(), 
        0,                   
        true,                // 开启遗嘱 Retain
        willPayload.c_str()  
    );

    if (success) {
        Serial.println(" 连接成功！");

        String onlinePayload = "{\"status\": \"online\"}";
        client.publish(topic_state.c_str(), onlinePayload.c_str(), true);

        client.subscribe(topic_command.c_str());
        Serial.println(" 已订阅指令主题: " + topic_command);

        // 连上之后第一件事：查水表，补发断网数据
        publishCachedData();

    } else {
        Serial.print(" 失败，错误码：");
        Serial.println(client.state());
    }
}

void publishStatusWithData(float temp, uint32_t freeHeap, uint32_t timestamp) {
    StaticJsonDocument<256> doc;
    
    doc["temp"] = serialized(String(temp, 1)); 
    doc["heap_kb"] = freeHeap / 1024; 

    uint32_t totalSeconds = timestamp / 1000;
    uint32_t days = totalSeconds / 86400;
    uint32_t hours = (totalSeconds % 86400) / 3600;
    uint32_t mins = (totalSeconds % 3600) / 60;
    uint32_t secs = totalSeconds % 60;
    
    char uptimeStr[32]; 
    sprintf(uptimeStr, "%dd %dh %dm %ds", days, hours, mins, secs);
    
    doc["uptime"] = uptimeStr;        
    doc["uptime_sec"] = totalSeconds; 

    doc["rssi"]   = WiFi.RSSI();
    doc["status"] = "online_OTA_V2";
    doc["ip"]     = WiFi.localIP().toString();

    char buffer[256]; 
    serializeJson(doc, buffer); 

    // 智能路由（有网发云，无网写盘）
    if(client.connected()) {
        boolean result = client.publish(topic_state.c_str(), buffer);
        if(result) {
            Serial.print("数据已推送到云端: ");
            Serial.println(buffer);
        } else {
            Serial.println("发布失败，转入本地缓存...");
            saveToCache(buffer); 
        }
    } else {
        Serial.println("当前处于断网状态，启动离线存储...");
        saveToCache(buffer);
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print(" 收到来自主题 [");
    Serial.print(topic);
    Serial.print("] 的指令: ");

    String messageTemp;
    for (int i = 0; i < length; i++) {
        messageTemp += (char)payload[i];
    }
    Serial.println(messageTemp);

    if (String(topic) != topic_command) {
        return; 
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, messageTemp);

    if (error) {
        Serial.print("JSON 解析失败: ");
        Serial.println(error.f_str());
        return;
    }

    // 路由分发：拦截并交给 OTA 模块处理
    if (doc["cmd"] == "ota") {
        String url = doc["url"].as<String>();
        triggerWanFOTA(url); 
        return; 
    }

    if (doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
        int r = doc["r"];
        int g = doc["g"];
        int b = doc["b"];

        rgb.setPixelColor(0, rgb.Color(r, g, b));
        rgb.show();
        
        Serial.printf("RGB 硬件已更新 -> R:%d, G:%d, B:%d\n", r, g, b);
    } else {
        Serial.println("格式错误：指令中缺少 r, g, b 参数！");
    }
}

void saveToCache(const char* data) {
    File file = LittleFS.open(CACHE_FILE, FILE_APPEND);
    if (!file) {
        Serial.println("无法打开缓存文件进行写入");
        return;
    }

    if (file.size() > MAX_CACHE_SIZE) {
        file.close();
        LittleFS.remove(CACHE_FILE); 
        Serial.println("缓存容量触顶 (512KB)！已清空历史沉淀，重新开始记录最新数据。");
        
        file = LittleFS.open(CACHE_FILE, FILE_WRITE);
        if (!file) {
            Serial.println("重建缓存文件失败！");
            return;
        }
    }

    file.println(data);
    file.close();
    Serial.println("数据已安全存入本地 Flash 缓存！");
}

void publishCachedData() {
    if (!LittleFS.exists(CACHE_FILE)) {
        return; 
    }

    File file = LittleFS.open(CACHE_FILE, FILE_READ);
    if (!file) {
        Serial.println("无法读取缓存文件");
        return;
    }

    Serial.println("网络恢复！开始补发历史断网数据...");
    int count = 0;
    
    while (file.available()) {
        String line = file.readStringUntil('\n'); 
        line.trim(); 
        
        if (line.length() > 0) {
            client.publish(topic_state.c_str(), line.c_str());
            count++;
            delay(50); 
        }
    }
    file.close();
    
    LittleFS.remove(CACHE_FILE); 
    Serial.printf("历史缓存清理完毕，共补发了 %d 条数据，文件已销毁。\n", count);
}