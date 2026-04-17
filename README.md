# 🚀 ESP32-S3 MQTT Boilerplate 

> 这是一个基于 ESP32-S3 的工业级轻量化 MQTT 接入模板（Station 模式 + 零外设）。不仅包含设备端高可用、高内聚的底层 C++ 实现，还打通了前往企业级 Java (Spring Boot) 物联网中台的全链路架构。

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20(PlatformIO)-orange.svg)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3--N16R8-red.svg)](https://www.espressif.com/)
[![Protocol](https://img.shields.io/badge/Protocol-MQTT%203.1.1-brightgreen.svg)]()

## 💡 项目简介

本项目旨在提供一个**真正可以作为生产级基石**的 ESP32 IoT 框架，拒绝面条式代码与“玩具级” Demo。

- **极简外设，极致压榨**：仅需一块 ESP32-S3 核心板，利用板载 Wi-Fi、RGB LED 与内部核心温度传感器即可完成数据闭环。
- **模块化架构 (HAL/SYS/CORE)**：严格的底层解耦，将硬件抽象(Sensor)、系统服务(OTA)与业务逻辑(MQTT)物理隔离，主循环代码零膨胀。
- **工业级网络韧性**：内置带超时防御的非阻塞式 Wi-Fi 状态机，以及 MQTT LWT (遗嘱机制) 与 Retain (保留消息)。
- **无感热更新 (OTA)**：内置 mDNS 服务发现与 ArduinoOTA，支持纯局域网无线刷机，彻底摆脱数据线束缚。

---

## 🏗️ 核心架构

### 阶段一：设备上云 (当前版本特性)
设备直接连接 MQTT Broker，实现双向 JSON 通信与状态自恢复：
* **心跳上报 (Pub)**：`device/esp32s3/{MAC}/state` (含 Uptime, 内部温度, RSSI, 局域网 IP 等)
* **命令监听 (Sub)**：`device/esp32s3/{MAC}/command` (RESTful 风格指令下发)

### 阶段二：全栈演进 (规划拓展)
打通至 `Java Spring Boot + Vue/JavaFX` 的全链路，实现真正的大型 IoT 中枢管理与数字孪生看板。
👉 **详细全栈架构设计请参阅 [docs/Architecture.md](docs/Architecture.md)**

---

## 📦 快速开始 (Quick Start)

### 1. 环境准备
- **硬件**：ESP32-S3-DevKitC-1 (推荐 N16R8) 或同类产品。
- **软件**：VS Code + PlatformIO 扩展。

### 2. 本地配置
打开 `include/config.h`，修改你的网络基础设施信息：
```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// 默认测试服务器，生产环境请替换为私有 EMQX/Mosquitto 集群
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_PORT 1883
```

### 3. 首次播种与 OTA 升级
**第一次烧录（有线）：**
通过 USB 连接电脑，使用 PlatformIO 的 `Upload` 功能进行基础烧录。

**后续热更新（无线 OTA）：**
首次烧录完成后，设备将在局域网广播 mDNS 域名。修改 `platformio.ini` 开启 OTA 模式：
```ini
upload_protocol = espota
upload_port = ESP32S3-YOUR_MAC.local  ; 或填入设备通过 MQTT 上报的 IP
upload_flags =
    --auth=admin123
```
点击 `Upload` 即可享受无感空中升级！

---

## 📊 数据协议规范 (Payload Format)

设备端默认采用严格的 JSON 序列化（基于 `ArduinoJson` 防止内存越界）。

**上报状态 (State) 示例：**
```json
{
  "uptime": 1205,
  "temp": 45.5,
  "rssi": -58,
  "ip": "192.168.43.100",
  "status": "online"
}
```

**控制指令 (Command) 示例 (控制板载 RGB)：**
```json
{
  "r": 0,
  "g": 255,
  "b": 0
}