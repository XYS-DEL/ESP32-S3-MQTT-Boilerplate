#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

//  回调函数：网卡每抓到一个包，就会触发一次这个中断 
void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  // 我们只过滤 Management Frame (管理帧)，排除掉别人看视频的巨大数据包
  if (type == WIFI_PKT_MGMT) { 
    
    // 强制指针转换：把无类型的内存块 buf，映射成底层的 802.11 封包结构体
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    
    // payload 指向了这个包真正的载荷数据
    uint8_t *payload = pkt->payload;
    
    // 从底层射频硬件获取这个包到达天线时的信号强度 (dBm)
    int rssi = pkt->rx_ctrl.rssi; 

    // 802.11 MAC 帧结构解析：
    // 第 0 个字节 (payload[0]) 是 Frame Control。
    // 0x40 这个十六进制数代表 Type 为 0 (管理帧)，Subtype 为 4 (Probe Request)
    if (payload[0] == 0x40) {
      
      // 在 802.11 协议中，Probe Request 的源 MAC 地址固定存放在从第 10 个字节开始的位置
      Serial.printf(" 捕获设备! MAC: %02X:%02X:%02X:%02X:%02X:%02X | 信号强度: %4d dBm\n",
                    payload[10], payload[11], payload[12],
                    payload[13], payload[14], payload[15], rssi);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- 启动 802.11 MAC 层无线电嗅探器 ---");

  // 1. 初始化 Wi-Fi 模块并断开所有连接
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // 2. 调用底层的 ESP-IDF API 开启混杂模式
  esp_wifi_set_promiscuous(true);
  
  // 3. 注册我们的回调函数，把网卡和解析逻辑绑死
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  
  Serial.println("🎧 混杂模式已开启，正在监听空气中的数据包...");
}

void loop() {
  // Wi-Fi 总共有 13 个信道 (Channel)
  // 如果一直停在一个信道，就抓不到别的信道上的手机。
  // 我们在主循环里写一个“全频段扫频”逻辑，每秒切换一次监听信道。
  static int current_channel = 1;
  esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
  
  // Serial.printf("正在扫描信道: %d\n", current_channel);
  
  current_channel = (current_channel % 13) + 1;
  delay(1000); 
}