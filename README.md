#  ESP32-S3 MQTT Boilerplate 
> 这是一个基于 ESP32-S3 的工业级轻量化 MQTT 接入模板（Station 模式 + 零外设）。不仅包含单片机端的高可用底层实现，还打通了前往企业级 Java (Spring Boot) 物联网中台的全链路架构。

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20(PlatformIO)-orange.svg)](https://platformio.org/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3--N16R8-red.svg)](https://www.espressif.com/)

##  项目简介
本项目旨在提供一个**真正可以作为生产级基石**的 ESP32 MQTT 框架，拒绝“玩具代码”。
- **0 外设**：一块 ESP32-S3 核心板（利用板载 WiFi 和 RGB LED）即可完成闭环。
- **工业级最佳实践**：内置非阻塞式重连、LWT(遗嘱机制)、Retain(保留消息)。
- **RESTful Topic 设计**：完美适配后端通配符订阅架构。

---

##  核心架构

### 阶段一：设备上云 (当前库代码)
设备连接公共 Broker，实现双向通信：
* 上报状态至：`device/esp32s3/{MAC}/state`
* 监听控制于：`device/esp32s3/{MAC}/command`

### 阶段二：全栈演进 (规划拓展)
打通至 `Spring Boot + Vue` 架构的全链路，实现真正的大型 IoT 中枢管理。
 **详细全栈架构设计请阅读 [docs/Architecture.md](docs/Architecture.md)**

---

##  快速开始 (Quick Start)

### 1. 环境准备
- 硬件：ESP32-S3-DevKitC-1 (N16R8) 或同类产品。
- 软件：VS Code + PlatformIO 插件。

### 2. 配置参数
打开 `src/main.cpp` (或相关的 config 文件)，修改以下宏定义：
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// 默认测试服务器，请勿传输敏感数据
const char* mqtt_broker = "broker.emqx.io";