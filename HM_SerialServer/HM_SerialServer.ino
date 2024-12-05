#include <M5Core2.h>
#include <esp_now.h>
#include <WiFi.h>

// デバイスのMACアドレス
uint8_t deviceMACs[][6] = {
    {0x08, 0xA6, 0xF7, 0x23, 0x13, 0x78},
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x08, 0xB6, 0x1F, 0x88, 0x6A, 0x98},
    {0x08, 0xB6, 0x1F, 0x88, 0x64, 0x14}
};

// メッセージ構造体
typedef struct struct_message {
  char message[50];
} struct_message;

struct_message myData;

// 表示行管理変数
int currentLine = 0;        // 現在の行
const int maxLines = 15;    // 画面に表示可能な最大行数

// 送信後のコールバック
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  String result = status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail";
  uint16_t color = status == ESP_NOW_SEND_SUCCESS ? GREEN : RED;

  M5.Lcd.setTextColor(color, BLACK);
  printToScreen("Send to Device [" + String((uint32_t)mac_addr[0], HEX) + "] - " + result);
}

// デバイス追加関数
void addDevice(const uint8_t *mac_addr) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac_addr, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    M5.Lcd.setTextColor(RED, BLACK);
    printToScreen("Failed to add device");
  } else {
    M5.Lcd.setTextColor(GREEN, BLACK);
    printToScreen("Device added successfully");
  }
}

// ブロードキャスト関数
void broadcast(const String &data) {
  strncpy(myData.message, data.c_str(), sizeof(myData.message));
  myData.message[sizeof(myData.message) - 1] = '\0';

  for (int i = 0; i < sizeof(deviceMACs) / sizeof(deviceMACs[0]); i++) {
    esp_now_send(deviceMACs[i], (uint8_t *)&myData, sizeof(myData));
  }

  M5.Lcd.setTextColor(WHITE, BLACK);
  printToScreen("Broadcast completed: " + data);
}

// 画面にテキストを表示する関数
void printToScreen(const String &text) {
  if (currentLine >= maxLines) {
    // 行数が上限に達したら画面をクリア
    M5.Lcd.clear();
    currentLine = 0;
  }
  M5.Lcd.setCursor(0, currentLine * 10);  // 1行あたりの高さは20ピクセル
  M5.Lcd.println(text);
  currentLine++;
}

// 初期化処理
void setup() {
  M5.begin();
  WiFi.mode(WIFI_STA); // ステーションモードに設定
  WiFi.disconnect();   // WiFiネットワークから切断

  M5.Lcd.println("ESP-NOW Sender");

  // ESP-NOW初期化
  if (esp_now_init() != ESP_OK) {
    M5.Lcd.setTextColor(RED, BLACK);
    printToScreen("ESP-NOW init failed");
    return;
  }
  esp_now_register_send_cb(onSent);

  // デバイスを追加
  for (int i = 0; i < sizeof(deviceMACs) / sizeof(deviceMACs[0]); i++) {
    addDevice(deviceMACs[i]);
  }
}

// メインループ
void loop() {
   if (M5.Touch.ispressed()) {
    broadcast("Eated");
    delay(150); // タップ後の遅延
  }
  if (Serial.available()) {
    String jsonData;
    while (Serial.available()) {
      char c = Serial.read();
      jsonData += c;
      delay(10);
    }

    // 受信したデータをブロードキャスト
    broadcast(jsonData);
    printToScreen("Broadcasting data: " + jsonData);
  }

  delay(300); // メインループの待機時間
}
