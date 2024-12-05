#include <M5Core2.h>
#include <WiFi.h>

void setup() {
  // M5Stack Core2 を初期化
  M5.begin();
  M5.Lcd.clear(BLACK);  // 画面を黒でクリア
  M5.Lcd.setTextColor(WHITE, BLACK);  // 白い文字、黒背景
  M5.Lcd.setTextSize(2);  // 文字サイズを設定

  // WiFi モードを設定
  WiFi.mode(WIFI_STA);  // ステーションモード
  WiFi.disconnect();    // 余分な接続を切断

  // MACアドレスを取得
  String macAddress = WiFi.macAddress();

  // MACアドレスを表示
  M5.Lcd.setCursor(10, 50);  // 表示位置を設定
  M5.Lcd.println("MAC Address:");
  M5.Lcd.setCursor(10, 80);  // 表示位置を変更してMACを表示
  M5.Lcd.println(macAddress);
}

void loop() {
  // ループ処理は必要なし
}
