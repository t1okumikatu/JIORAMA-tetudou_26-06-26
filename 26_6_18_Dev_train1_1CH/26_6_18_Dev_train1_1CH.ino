#include <esp_now.h>
#include <WiFi.h>
#include "constant.h"

// ★ここで実体を宣言する（externを付けず、値を代入する）
volatile int16_t Send1 = 0;
volatile int Send2=0, Send3=0, Send4=0, ctr=0;
volatile int Train1=0, Train2=0, Train3=0, Train4=0, Data5=0, Data6=0, Data7=0;

void setup() {
  // ... 今まで通りのセットアップ内容 ...
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // ★これを入れると受信が安定することが多いです
  // ... WiFi.mode(WIFI_STA); の後で ...
  Serial.print("Current MAC Address: ");
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(esp_now_recv_cb_t(onReceive));
  
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  ledcAttachChannel(in1, freq, resolution, ch1);
  ledcAttachChannel(in2, freq, resolution, ch2);
}

void loop() {
  forward(150); 
  // 受信側（Train）のシリアルモニタで確認
Serial.print("Current Channel: ");
Serial.println(WiFi.channel());
  Serial.println(Send1);
  if(Send1>0){
   forward(Send1); 
  }
  if(Send1==0){
   brake(); 
  }
  
  delay(20);
}