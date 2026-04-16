// src/iot_core.cpp
#include "iot_core.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// --- 全局变量声明  ---
String clientId, topic_state, topic_command;

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_NeoPixel rgb(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastReconnectAttempt = 0;
unsigned long lastStatusPublish = 0;

// --- 内部私有函数声明 ---
void setupWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectToMQTT();
void publishStatus();

// ==========================================
// 对外暴露的 Setup 函数
// ==========================================
void setupIoTSystem() {
  rgb.begin();
  rgb.setPixelColor(0, rgb.Color(0, 0, 0));
  rgb.show();

  setupWiFi();

  String mac = WiFi.macAddress();

  // 去除mac地址的：,符合mqtt的规范
  mac.replace(":", ""); 

  clientId = "ESP32S3-" + mac;
  topic_state = "device/esp32s3/" + clientId + "/state";
  topic_command = "device/esp32s3/" + clientId + "/command";

  Serial.println("\n--- MQTT Topic ---");
  Serial.println("状态上报: " + topic_state);
  Serial.println("命令下发: " + topic_command);

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(mqttCallback);
}

// ==========================================
// 对外暴露的 Loop 函数
// ==========================================
void loopIoTSystem() {
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      connectToMQTT(); 
    }
  } else {
    client.loop(); 
    
    unsigned long now = millis();
    if (now - lastStatusPublish > 10000) {
      lastStatusPublish = now;
      publishStatus();
    }
  }
}

// ==========================================
//  具体业务实现
// ==========================================

void setupWiFi() {
    int i = 0;

    WiFi.disconnect(true,true);
    delay(100);

    WiFi.mode(WIFI_STA); // 强制锁定STA模式
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);

    Serial.print(" 正在连接 WiFi... ");

    while(WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
        if(++i >= 10) {
            Serial.println("连接超时！请检查 WiFi 状况！");
            return;
        }
    }

        Serial.println(" WiFi 连接成功！");
        Serial.print(" IP 地址为: ");
        Serial.println(WiFi.localIP());
}

void connectToMQTT() {
    Serial.print("尝试连接 MQTT Broker...");
    
    //  准备好的内容
    String willPayload = "{\"status\": \"offline\"}";
    
    //  发起连接
    // 参数顺口溜：ID、账号、密码、遗嘱主题、QoS、Retain、遗嘱内容
    boolean success = client.connect(
        clientId.c_str(),    // 使用动态生成的 ID
        NULL,                // 公共服务器无账号
        NULL,                // 公共服务器无密码
        topic_state.c_str(), // 遗嘱发到状态主题
        0,                   // QoS 0 
        true,                //  必须为 true！这样别人一上线就能看到你挂了
        willPayload.c_str()  // 遗嘱内容
    );

    if (success) {
        Serial.println(" 连接成功！");

        // 亲自发布第一条“上线声明”，并开启 Retain
        String onlinePayload = "{\"status\": \"online\"}";
        client.publish(topic_state.c_str(), onlinePayload.c_str(), true);

        // 订阅指令主题，准备接收手机端的命令
        client.subscribe(topic_command.c_str());
        Serial.println(" 已订阅指令主题: " + topic_command);

    } else {
        Serial.print(" 失败，错误码：");
        Serial.println(client.state());
    }
}

void publishStatus() {
    // 开辟函数栈来存信息
    StaticJsonDocument<200> doc;
    
    // 写数据
    doc["uptime"] = millis() / 1000;
    doc["rssi"] = WiFi.RSSI(); // 信号强度
    doc["status"] = "online"; // 状态信息

    char buffer[200]; // 定义数组接收
    
    serializeJson(doc, buffer); // 封装数据

    boolean result = client.publish(topic_state.c_str(), buffer);

    if(result) {
        Serial.print("已发布: ");
        Serial.println(buffer);
    } else {
        Serial.println("发布失败 (可能未连接)");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print(" 收到来自主题 [");
    Serial.print(topic);
    Serial.print("] 的指令: ");

    // 这里的 payload 是 byte 数组（二进制流），不是字符串
    // 所以不能直接打印，也不能直接给 JSON 库用
    String messageTemp;
    for (int i = 0; i < length; i++) {
        messageTemp += (char)payload[i];
    }
    Serial.println(messageTemp);

    // 身份校验：虽然只订阅了 command，为了安全性验证一下主题
    // 如果订阅多个topic必须验证！
    if (String(topic) != topic_command) {
        return; 
    }

    // 核心解析：反序列化 JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, messageTemp);

    if (error) {
        Serial.print(" JSON 解析失败: ");
        Serial.println(error.f_str());
        return;
    }

    // 业务逻辑与容错：确保 JSON 里真的包含 r, g, b 这三个键
    if (doc.containsKey("r") && doc.containsKey("g") && doc.containsKey("b")) {
        // 取出数值
        int r = doc["r"];
        int g = doc["g"];
        int b = doc["b"];

        // 驱动底层硬件
        rgb.setPixelColor(0, rgb.Color(r, g, b));
        rgb.show();
        
        Serial.printf(" RGB 硬件已更新 -> R:%d, G:%d, B:%d\n", r, g, b);
    } else {
        Serial.println(" 格式错误：指令中缺少 r, g, b 参数！");
    }
}