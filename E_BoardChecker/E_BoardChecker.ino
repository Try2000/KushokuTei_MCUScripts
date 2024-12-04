#include <driver/dac.h>
#include <math.h>
#include <esp_now.h>
#include <WiFi.h>

// 定数
#define DAC_PIN_POS DAC_CHANNEL_1  // GPIO25
#define DAC_PIN_NEG DAC_CHANNEL_2  // GPIO26
#define SAMPLES 50                 // 1周期あたりのサンプル数
#define SAMPLING_INTERVAL 33       // サンプリング間隔 (µs) を33µsに設定
#define FREQUENCY 300              // 正弦波の周波数（300Hz）
#define SAMPLE_RATE 30000          // サンプリングレート（30kHz）

typedef struct struct_message {
  char message[50];
} struct_message;

struct_message receivedData;

// 正弦波データ
uint8_t sine_wave_pos[SAMPLES];
uint8_t sine_wave_neg[SAMPLES];
int sample_index = 0;
bool isOutput = false;  // 出力を制御するフラグ
unsigned long lastOutputTime = 0;  // 電流出力開始時間
const unsigned long outputDuration = 1000; // 電流出力時間 (1秒)

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  // 受信したデータを構造体にコピーする
  memcpy(&receivedData, data, sizeof(receivedData));
  changeOutputState(true);
}

// 正弦波データを生成する関数
void generate_sine_wave() {
  for (int i = 0; i < SAMPLES; i++) {
    float value = sin(2 * PI * i / SAMPLES);
    if (value >= 0) {
      sine_wave_pos[i] = (uint8_t)(178 * value);  // 正領域
      sine_wave_neg[i] = 0;
    } else {
      sine_wave_pos[i] = 0;
      sine_wave_neg[i] = (uint8_t)(178 * -value);
    }
  }
}

void setup() {
  Serial.begin(115200);  // シリアル通信の初期化
  Serial.println("準備完了: 'q'で電流出力をトグル");

  WiFi.mode(WIFI_STA);  // STAモードであることを確認
  WiFi.disconnect();    // デバイスがどのWi-Fiネットワークにも接続しないようにする
   if (esp_now_init() == ESP_OK) {
        Serial.println("ESP-Now Init Success");
    }
  esp_now_register_recv_cb(onReceive);

  dac_output_enable(DAC_PIN_POS);
  dac_output_enable(DAC_PIN_NEG);

  // DAC出力の初期値を0に設定
  dac_output_voltage(DAC_PIN_POS, 0);
  dac_output_voltage(DAC_PIN_NEG, 0);

  generate_sine_wave();
}

void changeOutputState(bool isoutput) {
  isOutput = isoutput;
  if (isOutput) {
    lastOutputTime = millis();  // 出力開始時間を記録
  }
  Serial.println(isOutput ? "電流出力ON" : "電流出力OFF");
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 'q') {  // 'q'が押されたら出力をトグル
      changeOutputState(!isOutput);
    }
    if (input == 'm') {
      // ESP32のMACアドレスを取得
      String macAddress = WiFi.macAddress();
      // シリアル通信でMACアドレスを送信
      Serial.println("ESP32 MAC Address: " + macAddress);
    }
  }

  // 正弦波出力
  if (isOutput) {
    // 電流出力を行う
    dac_output_voltage(DAC_PIN_POS, sine_wave_pos[sample_index]);
    dac_output_voltage(DAC_PIN_NEG, sine_wave_neg[sample_index]);

    // サンプルインデックスを更新
    sample_index = (sample_index + 1) % SAMPLES;

    // サンプリング間隔を保つための遅延
    delayMicroseconds(SAMPLING_INTERVAL);  // サンプリング間隔を維持

    // 1秒経過後に出力をオフ
    if (millis() - lastOutputTime >= outputDuration) {
      changeOutputState(false);
    }
  } else {
    // 出力をオフにする
    dac_output_voltage(DAC_PIN_POS, 0);
    dac_output_voltage(DAC_PIN_NEG, 0);
  }
}
