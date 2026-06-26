#ifndef CONSTANT_H
#define CONSTANT_H
#include <Arduino.h>

// --- 定数 ---
const int VALUE_MAX = 255; 
const int in1 = 23;
const int in2 = 22;
const int freq = 100; 
#define resolution 8
const int ch1 = 1;
const int ch2 = 2;

// --- 受信変数の「予約（extern）」 ---
// ここでは値を代入せず、extern を付けるだけにする
extern volatile int16_t Send1;
extern volatile int Send2, Send3, Send4, ctr;
extern volatile int Train1, Train2, Train3, Train4, Data5, Data6, Data7;

#endif