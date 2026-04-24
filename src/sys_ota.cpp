// src/sys_ota.cpp
#include "sys_ota.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>

// 广域网 FOTA 异步控制标志位 (只有这个文件内部能看见)
static bool isFotaTriggered = false;
static String targetFotaUrl = "";

// 内部私有函数：执行广域网下载与擦写
void executeFOTA(String downloadUrl) {
    Serial.println("\n=========================================");
    Serial.println("🚀 [广域网 FOTA] 开始执行空中升级...");
    Serial.println("🔗 目标固件地址: " + downloadUrl);
    Serial.println("=========================================\n");

    WiFiClient httpClient;
    
    httpUpdate.onStart([]() { Serial.println("🔽 开始下载新固件并擦除老旧 Flash 分区..."); });
    httpUpdate.onEnd([]() { Serial.println("\n✅ 固件刷写成功！系统即将涅槃重启..."); });
    httpUpdate.onProgress([](int cur, int total) {
        Serial.printf("⏳ 刷写进度: %d%%\r", (cur * 100) / total);
    });
    httpUpdate.onError([](int err) {
        Serial.printf("\n❌ 升级失败！错误码: %d\n", err);
    });

    t_httpUpdate_return ret = httpUpdate.update(httpClient, downloadUrl);

    if (ret == HTTP_UPDATE_FAILED) {
        Serial.printf("❌ FOTA 发生致命错误 (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    }
}

// 暴露给外部的函数 1：初始化局域网 OTA
void setupOTA(const String& hostname) {
    if (!MDNS.begin(hostname.c_str())) {
        Serial.println("mDNS 启动失败");
    } else {
        Serial.println("mDNS 服务已启动: " + hostname + ".local");
    }

    ArduinoOTA.setHostname(hostname.c_str()); 
    ArduinoOTA.setPassword("admin123"); 

    ArduinoOTA.onStart([]() { Serial.println(" 远程升级开始..."); });
    ArduinoOTA.onEnd([]() { Serial.println("\n 升级完成，重启中..."); });
    ArduinoOTA.onError([](ota_error_t error) { Serial.printf(" OTA错误[%u]\n", error); });

    ArduinoOTA.begin();
    Serial.println(" OTA 服务已就绪");
    MDNS.addService("arduino", "tcp", 3232);
}

// 暴露给外部的函数 2：MQTT 收到指令时调用这个
void triggerWanFOTA(const String& downloadUrl) {
    targetFotaUrl = downloadUrl;
    isFotaTriggered = true; 
    Serial.println("⚠️ [OTA 调度器] 收到云端指令，已排入下一次主循环执行序列！");
}

// 暴露给外部的函数 3：放在主程序 loop 里
void loopOTA() {
    // 1. 处理局域网推送
    ArduinoOTA.handle();

    // 2. 处理广域网拉取
    if (isFotaTriggered) {
        isFotaTriggered = false; // 降下标志位
        executeFOTA(targetFotaUrl); 
    }
}