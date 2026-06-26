#define EXTERN_MAIN
#include <esp_now.h>
#include <WiFi.h>
#include "constant.h"

// 💡 競合を防ぐため、sendData と recvData の実体定義は constant.h 側のルールに従う
unsigned long lastVoltageTime = 0;
volatile uint32_t targetSpeed2 = 0; 
float currentSpeed2 = 0.0;

// ⏱️ 遅延計測用の変数
unsigned long stopSignalReceivedTime = 0;
float lastMeasuredDelay = 0.0;
bool readyToSendDelay = false;

// === 🎯 最速電波受信コールバック（v3.3.7 互換・安全パース版） ===
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
#endif
  if (len >= sizeof(CombinedPayload)) {
    CombinedPayload temp;
    memcpy(&temp, incomingData, sizeof(CombinedPayload));
    
    // 🟢 ステーションからの正規の通常運行電波（header == 99 かつ data88 == 88）だけを確実に処理
    if (temp.header == 99 && temp.data88 == 88) {
      
      // ⏱️ 走行中（targetSpeed2 > 0）に、新しく「速度0（停止指示）」が届いた【瞬間】だけを確実に検知！
      if (targetSpeed2 > 0 && temp.train2_speed == 0) {
        stopSignalReceivedTime = millis();
        
        // 🔥【超即応停止】
        targetSpeed2 = 0;
        currentSpeed2 = 0.0;
        brake();
        
        // 📡【ステーションへの高速アンサーバック（77）】
        CombinedPayload ackPack;
        ackPack.header = 99;
        ackPack.data88 = 77; 
        ackPack.train2_speed = 0;
        esp_now_send(broadcastAddress, (uint8_t *)&ackPack, sizeof(ackPack));
        
        // ⏱️ 列車内部の処理タイムラグ計算
        unsigned long diff = millis() - stopSignalReceivedTime;
        lastMeasuredDelay = (float)diff;
        readyToSendDelay = true; // loop側へモニター（66）送信を指示

        lastVoltageTime = millis(); // 電圧タイマーをリセット
      } else {
        // 🟢 通常の走行指示（速度が1以上、または停止指示が継続しているとき）
        // 受信した速度をそのまま正しくターゲット速度へ反映！
        targetSpeed2 = temp.train2_speed;
      }

      recvData = temp;
    }
  }
}

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {}

void setup() {
  Serial.begin(115200);
  pinMode(in1, OUTPUT); 
  pinMode(in2, OUTPUT); 
  pinMode(adcPin, INPUT);

  ledcAttach(in1, freq, resolution);
  ledcAttach(in2, freq, resolution);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  StatiNowpeer();
  
  lastVoltageTime = millis(); // タイマースタート
  Serial.println("--- 列車2：電波受信＆モーター回転両立 確定版 起動完了 ---");
}

void loop() {
  // ⏱️ 1. 停止指示があった場合は、その「内部遅延ms」をモニターへ即時送信
  if (readyToSendDelay) {
    readyToSendDelay = false;
    
    sendData.header = 99;
    sendData.data88 = 66; // 列車2内部遅延専用コード
    sendData.voltage = lastMeasuredDelay; 
    
    esp_now_send(broadcastAddress, (uint8_t *)&sendData, sizeof(sendData));
    Serial.print("【📡 送信】列車2内部遅延: ");
    Serial.println(lastMeasuredDelay, 0);
  }

  // 🔋 2. 定期的なバッテリー電圧送信（readV.ino）
  readV();

  // ⚙️ 3. モーターの加減速処理と回転指示
  if (targetSpeed2 > 0) {
    // 指示された速度に向けてスムーズに加速（しゃくり防止）
    currentSpeed2 += ((float)targetSpeed2 - currentSpeed2) * 0.25;
    forward((uint32_t)(currentSpeed2 + 0.5));
  } else {
    // 速度指示が0の時はブレーキ（停止）を維持
    brake();
    currentSpeed2 = 0.0;
  }
  
  delay(20); 
}