#include <driver/dac.h>
#include <math.h>

// 定数
#define DAC_PIN_POS DAC_CHANNEL_1  // GPIO25
#define DAC_PIN_NEG DAC_CHANNEL_2  // GPIO26
#define SAMPLES 50                // 1周期あたりのサンプル数
#define SAMPLING_INTERVAL 33     // サンプリング間隔 (µs) を33µsに設定
#define FREQUENCY 300              // 正弦波の周波数（300Hz）
#define SAMPLE_RATE 30000          // サンプリングレート（30kHz）

// 正弦波データ
uint8_t sine_wave_pos[SAMPLES];
uint8_t sine_wave_neg[SAMPLES];
int sample_index = 0;
bool output_enabled = false;  // 出力を制御するフラグ

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

    dac_output_enable(DAC_PIN_POS);
    dac_output_enable(DAC_PIN_NEG);

    // DAC出力の初期値を0に設定
    dac_output_voltage(DAC_PIN_POS, 0);
    dac_output_voltage(DAC_PIN_NEG, 0);

    generate_sine_wave();
}

void loop() {
    if (Serial.available() > 0) {
        char input = Serial.read();
        if (input == 'q') {  // 'q'が押されたら出力をトグル
            output_enabled = !output_enabled;
            Serial.println(output_enabled ? "電流出力ON" : "電流出力OFF");
        }
    }

    // 正弦波出力
    if (output_enabled) {
        dac_output_voltage(DAC_PIN_POS, sine_wave_pos[sample_index]);
        dac_output_voltage(DAC_PIN_NEG, sine_wave_neg[sample_index]);
        
        // サンプルインデックスを更新
        sample_index = (sample_index + 1) % SAMPLES;
        
        // サンプリング間隔を保つための遅延
        delayMicroseconds(SAMPLING_INTERVAL);  // サンプリング間隔を維持
    } else {
        // 出力をオフにする
        dac_output_voltage(DAC_PIN_POS, 0);
        dac_output_voltage(DAC_PIN_NEG, 0);
    }
}
