#include "constant.h"
#include <esp_now.h>

// メイン側で実体化したタイマー変数を外部参照して共有
extern unsigned long lastVoltageTime;
const unsigned long voltageInterval = 10000; // 10秒間隔

float readBattery() {
  // 🔋 オリジナルのピン読み込みと計算式
  int adc = analogRead(adcPin); 
  float v = (adc / 4095.0) * 1.7 * 5.0;  
  return v; 
}

void readV() {
  if (millis() - lastVoltageTime >= voltageInterval) {
    lastVoltageTime = millis();
    
    float currentVolt = readBattery();
    
    Serial.print("【🔋 電圧送信】: ");
    Serial.print(currentVolt);
    Serial.println(" V");

    // モニター機に送るためにパッキング
    sendData.header = 99;
    sendData.data88 = 88;            // 💡【重要】0から88に修正！モニター機に通常電圧データとして正しくパースさせる
    sendData.voltage = currentVolt; // バッテリー電圧を格納
    
    // ESP-NOWで送信
    esp_now_send(broadcastAddress, (uint8_t *)&sendData, sizeof(sendData));
  }
}