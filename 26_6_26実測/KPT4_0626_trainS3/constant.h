#ifndef CONSTANT_H
#define CONSTANT_H

#include <Arduino.h>

// 📡 ブロードキャストアドレス
static const uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ⚙️ モーター制御・ピン設定用の定数
#ifndef VALUE_MAX
#define VALUE_MAX 255
#endif

const int in1 = 1;       // モーター制御ピン1
const int in2 = 2;       // モーター制御ピン2
const int adcPin = 3;    // バッテリー電圧測定用ピン
const int freq = 5000;    // PWM周波数
const int resolution = 8; // PWM解像度（8bit: 0〜255）

// ==========================================================
// 🎯 【送受信共通】24バイト統合構造体（変更禁止）
// ==========================================================
struct CombinedPayload {
  uint8_t header;       // data[0]  (常に 99)
  uint8_t train1_speed; // data[1]
  uint8_t fb_light3;    // data[2]
  uint8_t train2_speed; // data[3]  (列車2のターゲット速度)
  uint8_t fb_light4;    // data[4]
  uint8_t train3_speed; // data[5]
  uint8_t data5_light;  // data[6]
  uint8_t train4_speed; // data[7]
  uint8_t data6_light;  // data[8]
  uint8_t train1_poji;  // data[9]
  uint8_t train2_poji;  // data[10]
  uint8_t train3_poji;  // data[11]
  uint8_t train4_poji;  // data[12]
  uint8_t Data5_SIn;    // data[13]
  uint8_t Data6_SOut;   // data[14]
  uint8_t Data7_Sub;    // data[15]
  uint8_t Data8_Main;   // data[16]
  uint8_t ctr;          // data[17]
  uint8_t startbutton;  // data[18]
  uint8_t data88;       // data[19]  (88:通常データ, 77:遅延アンサーバック, 66:内部遅延)

  float voltage;        // バッテリー電圧 または 計測された遅延時間ms
} __attribute__((packed));

// ==========================================================
// ⚠️ 重複定義（Multiple Definition）を完全に防ぐ魔法の記述
// ==========================================================
#ifdef EXTERN_MAIN
  // メインのタブ（KPT4_0626_trainS3.ino）側で実体を定義
  CombinedPayload sendData;
  CombinedPayload recvData;
#else
  // それ以外のサブタブ（readV.ino や CONTROL.ino）側からはこれらを参照する
  extern CombinedPayload sendData;
  extern CombinedPayload recvData;
#endif

// 🛠️ 別タブにある関数の外部宣言（コンパイル警告対策）
extern void forward(uint32_t pwm);
extern void brake();
extern void readV();
extern void StatiNowpeer();

#endif /* CONSTANT_H */