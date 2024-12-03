//--------------------------------------------------------------------------------
//  M5Stack Core2 冷却ファンPWM制御　テストプログラム.
//  中央ボタンを押したときのみデューティ比で動作させる仕様.
//--------------------------------------------------------------------------------

#include <M5Core2.h>
#include <esp_now.h>
#include <WiFi.h>
#include <AXP192.h>

const uint8_t FAN_PWM_PIN     = 26;
const uint8_t FAN_SENSOR_PIN  = 19;

int display_rotation = 1;
String duty_str = "";
int duty = 0;

const uint8_t CH0       = 0;
const double  PWM_FREQ  = 25000;
const uint8_t PWM_BIT   = 8;
int cur_duty_val = 0;
const unsigned long resetDelay = 5000; // Define resetDelay (example: 5000 milliseconds)

unsigned long lastPulse_T;
unsigned long pulse_Interval = 0;

AXP192 power;
typedef struct struct_message { //sketch_aug15a_client.inoより流用
  char message[50];
} struct_message;

struct_message receivedData;
bool receivedFlag = false;

void Send_PWM(int cur_duty_val) {
  ledcWrite(CH0, cur_duty_val);
}

unsigned long lastReceivedTime = 0;
// sketch_aug15a_client.inoより流用
// メッセージを受信した際のコールバック関数
void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  // 受信したデータを構造体にコピーする
  memcpy(&receivedData, data, sizeof(receivedData));
  receivedFlag = true;  // 受信フラグを立てる

  //receivedData.message[sizeof(receivedData) - 1] = '\0';  // 文字列の終端を保証

  // 受信したデータが「open」ならば receivedFlag を true にし、「Eated」ならば false にする
  // if (strcmp(receivedData.message, "open") == 0) {
  //   receivedFlag = true;
  //   lastReceivedTime = millis(); 
  // } else if (strcmp(receivedData.message, "Eated") == 0) {
  //   receivedFlag = false;
  // }
}

void display_output_duty(int cur_duty_val, bool isRunning) {
  if (cur_duty_val < 10) {                        
    duty_str = "00" + String(cur_duty_val);
  } else if (cur_duty_val == 100) {
    duty_str = String(cur_duty_val);
  } else {
    duty_str = "0" + String(cur_duty_val);
  }

  M5.Lcd.fillRect(150, 75, 100, 50, BLACK);
  
  if (isRunning) {
    M5.Lcd.setTextColor(GREEN, BLACK);  // 動作中は緑色
  } else {
    M5.Lcd.setTextColor(RED, BLACK);    // 停止中は赤色
  }
  
  M5.Lcd.setCursor(150, 75, 7);
  M5.Lcd.print(duty_str);
}

void setup() {
  M5.begin(true, true, true, false, kMBusModeOutput);
  M5.Lcd.setRotation(display_rotation);

  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(5, 5, 4);
  M5.Lcd.print("Cooling Fan PWM Ctrl");
  
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(5, 75, 4);
  M5.Lcd.print("DutyRate:");
  M5.Lcd.setCursor(260, 105, 2);
  M5.Lcd.print("%");

  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(5, 210, 2);
  M5.Lcd.printf("Battery:");
  M5.Lcd.setCursor(280, 210, 2);
  M5.Lcd.print("%");

  Serial.begin(115200);
  delay(500);

  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_SENSOR_PIN, INPUT);

  ledcSetup(CH0, PWM_FREQ, PWM_BIT);
  ledcAttachPin(FAN_PWM_PIN, CH0);
  Send_PWM(cur_duty_val);

  lastPulse_T = 0;
  pulse_Interval = 0;

  // Wi-FiとESP-NOWの初期化
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  //M5.Lcd.println("ESP-NOW Receiver");

  if (esp_now_init() != ESP_OK) {
    M5.Lcd.println("Error initializing ESP-NOW");
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {
  static bool touch = false;
  TouchPoint_t pos = M5.Touch.getPressPoint();
  static unsigned long pressTime = 0;
  static bool centerButtonPressed = false;
  float vol = power.GetBatVoltage();
  float batt = (vol - 3.7)/1.4 * 100;

  M5.Lcd.fillRect(70, 210, 100, 50, WHITE);
  M5.Lcd.fillRect(70, 210, 100 * batt / 100, 50, GREEN);
  if(batt < 1){
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.fillRect(200, 210, 80, 50, BLACK);
    M5.Lcd.setCursor(200, 210, 2);
    M5.Lcd.print("Low Battery");
  }else{
    M5.Lcd.fillRect(200, 210, 80, 50, BLACK);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setCursor(200, 210, 2);
    M5.Lcd.print(batt);
  }

  // タッチ入力処理
  if (pos.x != -1) {
    if (!touch) {
      touch = true;
      M5.Axp.SetLed(false);
    }
    if (pos.y > 240) {
      if (pos.x < 109) {
        if (cur_duty_val > 0) {
          cur_duty_val = cur_duty_val - 5;
          M5.Axp.SetLDOEnable(3, true);
          delay(200);
          M5.Axp.SetLDOEnable(3, false);
        }
      } else if (pos.x > 218) {
        if (cur_duty_val < 100) {
          cur_duty_val = cur_duty_val + 5;
          M5.Axp.SetLDOEnable(3, true);
          delay(200);
          M5.Axp.SetLDOEnable(3, false);
        }        
      } else if (pos.x >= 109 && pos.x <= 218) {
        pressTime = millis();
        centerButtonPressed = true;
      }
    }
    display_output_duty(cur_duty_val, centerButtonPressed);  
  }

  // 受信データに基づく処理
  if (receivedFlag) {
    pressTime = millis();
    centerButtonPressed = true;
    receivedFlag = false;  // フラグをリセット
  }
  // if (receivedFlag & (millis() - lastReceivedTime >= resetDelay)) {
  //   Send_PWM(0);
  //     receivedFlag = false;  // フラグをリセット
  //     centerButtonPressed = false;
  // }
  // ファンの回転制御
  if (centerButtonPressed) {
    unsigned long elapsedTime = millis() - pressTime;

    if (elapsedTime >= 3000) {
      Send_PWM(0);
      receivedFlag = false;  // フラグをリセット
      centerButtonPressed = false;
    } else {
      Send_PWM(256 * cur_duty_val / 100);
    }
    display_output_duty(cur_duty_val, centerButtonPressed);
  } else {
    display_output_duty(cur_duty_val, false);  // 停止時の表示更新
  }

  
}
