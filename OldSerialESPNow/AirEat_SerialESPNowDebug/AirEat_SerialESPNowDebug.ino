#include <M5Core2.h>
#include <esp_now.h>
#include <WiFi.h>

// 定义接收设备的 MAC 地址 / 受信デバイスのMACアドレスを定義
uint8_t receiver1MAC[] = {0x08, 0xB6, 0x1F, 0x88, 0x7C, 0x54}; // 替换为接收设备1的实际MAC地址 / 受信デバイス1の実際のMACアドレスに置き換える
uint8_t receiver2MAC[] = {0x30, 0xC6, 0xF7, 0x24, 0x33, 0xDC}; // 替换为接收设备2的实际MAC地址 / 受信デバイス2の実際のMACアドレスに置き換える
uint8_t receiver3MAC[] = {0x7C, 0x9E, 0xBD, 0x36, 0xD5, 0x50}; // 替换为接收设备3的实际MAC地址 / 受信デバイス3の実際のMACアドレスに置き換える
uint8_t receiver4MAC[] = {0x08, 0xB6, 0x1F, 0x88, 0x6A, 0x98};
uint8_t receiver5MAC[] = {0x08, 0xB6, 0x1F, 0x88, 0x64, 0x14};

// 定义发送的消息结构体 / 送信するメッセージ構造体を定義
typedef struct struct_message {
  char message[50]; // 消息内容 / メッセージ内容
} struct_message;

struct_message myData;

// 发送完成后的回调函数 / 送信完了後のコールバック関数
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  M5.Lcd.printf("Last Packet Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  M5.begin();
  WiFi.mode(WIFI_STA); // 确保处于 STA 模式 / STAモードに設定
  WiFi.disconnect();   // 确保设备不连接到任何 Wi-Fi 网络 / Wi-Fiネットワークから切断して、ESP-NOWモードを有効にする

  M5.Lcd.println("ESP-NOW Sender");

  // 初始化 ESP-NOW / ESP-NOWの初期化
  if (esp_now_init() != ESP_OK) {
    M5.Lcd.println("Error initializing ESP-NOW / ESP-NOWの初期化エラー");
    return;
  }

  esp_now_register_send_cb(onSent); // 注册发送回调函数 / 送信コールバック関数を登録

  // 添加第一个接收设备 / 受信デバイス1を追加
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); // 清空结构体 / 構造体をクリア

  memcpy(peerInfo.peer_addr, receiver1MAC, 6); // 设置接收设备1的 MAC 地址 / 受信デバイス1のMACアドレスを設定
  peerInfo.channel = 1;  // 显式指定频道为1 / チャンネルを1に設定
  peerInfo.encrypt = false; // 不加密 / 暗号化なし

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    M5.Lcd.println("Failed to add peer 1 ");
  } else {
    M5.Lcd.println("Peer 1 added successfully");
  }

  // 添加第二个接收设备 / 受信デバイス2を追加
  memcpy(peerInfo.peer_addr, receiver2MAC, 6); // 设置接收设备2的 MAC 地址 / 受信デバイス2のMACアドレスを設定
  peerInfo.channel = 1;  // 使用相同的频道 / 同じチャンネルを使用

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    M5.Lcd.println("Failed to add peer 2");
  } else {
    M5.Lcd.println("Peer 2 added successfully");
  }

  // 添加第三个接收设备 / 受信デバイス3を追加
  memcpy(peerInfo.peer_addr, receiver3MAC, 6); // 设置接收设备3的 MAC 地址 / 受信デバイス3のMACアドレスを設定
  peerInfo.channel = 1;  // 使用相同的频道 / 同じチャンネルを使用

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    M5.Lcd.println("Failed to add peer 3");
  } else {
    M5.Lcd.println("Peer 3 added successfully");
  }

  memcpy(peerInfo.peer_addr, receiver4MAC, 6); // 设置接收设备3的 MAC 地址 / 受信デバイス3のMACアドレスを設定
  peerInfo.channel = 1;  // 使用相同的频道 / 同じチャンネルを使用

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    M5.Lcd.println("Failed to add peer 4");
  } else {
    M5.Lcd.println("Peer 4 added successfully");
  }

  memcpy(peerInfo.peer_addr, receiver5MAC, 6); // 设置接收设备3的 MAC 地址 / 受信デバイス3のMACアドレスを設定
  peerInfo.channel = 1;  // 使用相同的频道 / 同じチャンネルを使用

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    M5.Lcd.println("Failed to add peer 5");
  } else {
    M5.Lcd.println("Peer 5 added successfully");
  }
}

void loop() {
  if (Serial.available()) {
    String jsonData = "";  // Stringでデータを一時的に格納
    while (Serial.available()) {
      char c = Serial.read();
      jsonData += c;
      delay(10);  // 読み取りの安定性を確保するための短い遅延
    }

    // データがmessageのサイズを超えないように制限する
    jsonData.toCharArray(myData.message, sizeof(myData.message));

    // 受け取ったデータを表示する
    M5.Lcd.setCursor(0, 40); // カーソルをさらに下に設定
    M5.Lcd.println(myData.message); // JSONデータを表示
    // 发送到第一个设备 / 受信デバイス1に送信
    esp_now_send(receiver1MAC, (uint8_t *)&myData, sizeof(myData));

  // 发送到第二个设备 / 受信デバイス2に送信
    esp_now_send(receiver2MAC, (uint8_t *)&myData, sizeof(myData));

  // 发送到第三个设备 / 受信デバイス3に送信
    esp_now_send(receiver3MAC, (uint8_t *)&myData, sizeof(myData));

    esp_now_send(receiver4MAC, (uint8_t *)&myData, sizeof(myData));

    esp_now_send(receiver5MAC, (uint8_t *)&myData, sizeof(myData));

    delay(100);  // 短い遅延を追加
  }

  delay(300);  // 每2秒发送一次 / 2秒ごとに送信
}
