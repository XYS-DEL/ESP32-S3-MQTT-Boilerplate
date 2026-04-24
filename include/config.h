// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// 网络配置 
// ==========================================
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ==========================================
// MQTT 配置
// ==========================================
#define MQTT_BROKER     "broker.emqx.io"
#define MQTT_PORT        8883         //MQTTS加密端口
// 证书字符组，自行去官网 https://www.emqx.com/zh/mqtt-dashboard 或者使用命令行 openssl s_client -connect broker.emqx.io:8883 -showcerts 获取
extern const char* root_ca;

// ==========================================
// 硬件引脚配置
// ==========================================
#define RGB_PIN         48
#define NUMPIXELS       1

#endif