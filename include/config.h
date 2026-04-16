// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// 网络配置 
// ==========================================
#define WIFI_SSID       "ssd"
#define WIFI_PASSWORD   "0987654321"

// ==========================================
// MQTT 配置
// ==========================================
#define MQTT_BROKER     "broker.emqx.io" // 连接不上请使用 broker-cn.emqx.io
#define MQTT_PORT       1883

// ==========================================
// 硬件引脚配置
// ==========================================
#define RGB_PIN         48
#define NUMPIXELS       1

#endif