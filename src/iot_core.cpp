// src/iot_core.cpp
#include <Arduino.h>
#include <stdint.h>
#include "iot_core.h"
#include "config.h"
#include "hal_sensor.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>

// --- 全局变量声明  ---
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

WiFiClientSecure espClient; 
PubSubClient client(espClient);

Adafruit_NeoPixel rgb(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastReconnectAttempt = 0;

// --- 内部私有函数声明 ---
void setupWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectToMQTT();

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
  // 1. 如果断线了，执行非阻塞重连
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      connectToMQTT(); 
    }
  } else {
    // 2. 维持 MQTT 底层心跳和接收下发指令
    // 发送数据全部移交给 FreeRTOS 队列处理了
    client.loop(); 
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

    Serial.print(" 正在同步网络时间(NTP)...");
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // 东八区时间

    time_t now = time(nullptr);
    while (now < 24 * 3600) { // 如果时间还没同步成功（小于一天）
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(" 时间同步完成！");

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

    espClient.setCACert(root_ca);
    Serial.println("TLS/SSL 根证书已加载，准备进行加密握手...");
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

void publishStatusWithData(float temp, uint32_t freeHeap, uint32_t timestamp) {
    StaticJsonDocument<256> doc;
    
    doc["temp"] = serialized(String(temp, 1)); 
    doc["heap_kb"] = freeHeap / 1024; 

    uint32_t totalSeconds = timestamp / 1000;
    uint32_t days = totalSeconds / 86400;
    uint32_t hours = (totalSeconds % 86400) / 3600;
    uint32_t mins = (totalSeconds % 3600) / 60;
    uint32_t secs = totalSeconds % 60;
    
    char uptimeStr[32]; // 分配一个小数组来装可读的时间
    sprintf(uptimeStr, "%dd %dh %dm %ds", days, hours, mins, secs);
    
    doc["uptime"] = uptimeStr;        // 时间
    doc["uptime_sec"] = totalSeconds; // 给数据库留的纯粹数字

    // 其他常规数据
    doc["rssi"]   = WiFi.RSSI();
    doc["status"] = "online_OTA_V2";
    doc["ip"]     = WiFi.localIP().toString();

    char buffer[256]; 
    serializeJson(doc, buffer); 

    // 发布到状态主题
    boolean result = client.publish(topic_state.c_str(), buffer);

    if(result) {
        Serial.print("数据已推送到云端: ");
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

    // 反序列化 JSON
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