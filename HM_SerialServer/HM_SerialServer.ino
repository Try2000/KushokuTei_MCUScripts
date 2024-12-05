#include <M5Core2.h>
#include <esp_now.h>
#include <WiFi.h>

// 定义接收设备的 MAC 地址
uint8_t receiver1MAC[] = {0x08, 0xB6, 0x1F, 0x88, 0x7C, 0x54};
uint8_t receiver2MAC[] = {0x30, 0xC6, 0xF7, 0x24, 0x33, 0xDC};
uint8_t receiver3MAC[] = {0x7C, 0x9E, 0xBD, 0x36, 0xD5, 0x50};
uint8_t receiver4MAC[] = {0x08, 0xB6, 0x1F, 0x88, 0x6A, 0x98};
uint8_t receiver5MAC[] = {0x08, 0xB6, 0x1F, 0x88, 0x64, 0x14};

// 广播地址
uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// 定义发送的消息结构体
typedef struct struct_message {
  char message[50];
} struct_message;

struct_message myData;

// 发送完成后的回调函数
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  M5.Lcd.printf("Last Packet Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  M5.begin();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  M5.Lcd.println("ESP-NOW Sender");

  if (esp_now_init() != ESP_OK) {
    M5.Lcd.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));

  // 添加所有目标设备
  uint8_t *receivers[] = {receiver1MAC, receiver2MAC, receiver3MAC, receiver4MAC, receiver5MAC};
  for (int i = 0; i < 5; i++) {
    memcpy(peerInfo.peer_addr, receivers[i], 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      M5.Lcd.printf("Failed to add peer %d\n", i + 1);
    } else {
      M5.Lcd.printf("Peer %d added successfully\n", i + 1);
    }
  }
}

void loop() {
  TouchPoint_t pos = M5.Touch.getPressPoint();

  if (pos.x >= 109 && pos.x <= 218) {
    // 广播 "Eated"
    M5.Lcd.clear();
    strncpy(myData.message, "Eated", sizeof(myData.message));
    esp_now_send(broadcastMAC, (uint8_t *)&myData, sizeof(myData));
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.printf("Broadcast: %s\n", myData.message);

    // 向所有指定设备发送 "1"
    strncpy(myData.message, "1", sizeof(myData.message));
    uint8_t *receivers[] = {receiver1MAC, receiver2MAC, receiver3MAC, receiver4MAC, receiver5MAC};
    for (int i = 0; i < 5; i++) {
      esp_now_send(receivers[i], (uint8_t *)&myData, sizeof(myData));
      M5.Lcd.printf("Sent to Peer %d: %s\n", i + 1, myData.message);
    }
    delay(10);
  }
  if (Serial.available()) {
    String jsonData;
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

    esp_now_send(receiver4MAC, (uint8_t *)&myData, sizeof(myData));
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.printf("Broadcast: %s\n", myData.message);

    strncpy(myData.message, "1", sizeof(myData.message));//開口時
    uint8_t *receivers[] = {receiver1MAC, receiver2MAC, receiver3MAC, receiver4MAC, receiver5MAC};
    for (int i = 0; i < 5; i++) {
      esp_now_send(receivers[i], (uint8_t *)&myData, sizeof(myData));
      M5.Lcd.printf("Sent to Peer %d: %s\n", i + 1, myData.message);
    }

    delay(100);  // 短い遅延を追加
  }

  delay(300); // 控制发送间隔
  M5.Lcd.setTextColor(RED, BLACK);
}
