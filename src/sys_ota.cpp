#include "sys_ota.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

void setupOTA(const String& hostname) {
    // 显式启动 mDNS 服务
    // 这会让板子在局域网内广播自己：“我是 [hostname].local”
    if (!MDNS.begin(hostname.c_str())) {
        Serial.println("mDNS 启动失败");
    } else {
        Serial.println("mDNS 服务已启动: " + hostname + ".local");
    }

    // 配置 OTA
    ArduinoOTA.setHostname(hostname.c_str()); 
    ArduinoOTA.setPassword("admin123"); // 你的无线刷机密码

    ArduinoOTA.onStart([]() { Serial.println(" 远程升级开始..."); });
    ArduinoOTA.onEnd([]() { Serial.println("\n 升级完成，重启中..."); });
    ArduinoOTA.onError([](ota_error_t error) { Serial.printf(" OTA错误[%u]\n", error); });

    ArduinoOTA.begin();
    Serial.println(" OTA 服务已就绪");

    // 向 mDNS 添加服务发现
    MDNS.addService("arduino", "tcp", 3232);
}

void loopOTA() {
    ArduinoOTA.handle();
}