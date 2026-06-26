#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "SPIFFS.h"

#include "constant.h" 

#define CHANNEL 0
#define PIN_PE7 25          
#define NANO_reset  27
#define EN_reset    35
#define PIN_PE0 4           
#define PIN_PE1 18          
#define PIN_PE2 23          
#define PIN_PE3 19          
#define StartSW 13
#define servoInt 15 

esp_now_peer_info_t slave;

// ⏱️ 飛行時間計測用の変数（メインタブ側だけで安全に使用するローカル変数として定義）
static unsigned long stopSignalSentTime = 0;
static bool isWaitingForTrain2Ack = false;

// === ① 受信コールバック（ライブラリv3.3.7最新仕様・ストップ時ダイレクト送信版） ===
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len >= sizeof(CombinedPayload)) {
    CombinedPayload temp;
    memcpy(&temp, incomingData, sizeof(CombinedPayload));
    
    // ⏱️ 列車2からの「ストップ届いたよ(77)」の光速アンサーバックを検知
    if (temp.header == 99 && temp.data88 == 77) {
      if (isWaitingForTrain2Ack) {
        isWaitingForTrain2Ack = false;
        
        // 往復時間(RTT)を計算
        unsigned long rtt = millis() - stopSignalSentTime;
        // 片道飛行時間は往復の半分(÷2)
        float oneWayDelay = (float)rtt / 2.0;
        
        Serial.println("\n====================================");
        Serial.print("【📡 ステーション実測】電波の片道飛行時間: ");
        Serial.print(oneWayDelay, 1);
        Serial.println(" ms (ミリ秒)");
        Serial.println("====================================");

        // ============================================================
        // 🎯 走行ストップの瞬間に、モニターへ向けてダイレクトに単発送信！
        // ============================================================
        CombinedPayload monitorPack = sendData; // 現在のステーションの状態をベースにする
        monitorPack.header = 99;
        monitorPack.data88 = 77;                // ⚡ ストップ確定フラグ
        monitorPack.voltage = oneWayDelay;      // ⏳ 計算した遅延msをそのまま乗せる
        
        esp_now_send(broadcastAddress, (uint8_t *)&monitorPack, sizeof(CombinedPayload));
      }
    } else {
      // 列車2から送られてくる10秒ごとの通常バッテリー電圧データはここに格納
      recvData = temp;
      currentFeedbackVoltage = recvData.voltage;
    }
  }
}

// === ② 送信完了コールバック（🎯エラー箇所：最新v3.3.7仕様のwifi_tx_info_tに完全修正） ===
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {}

// プロトタイプ宣言
void Receive();
void in_Servo_Contlor();
void out_Servo_Contlor();
void sankaku();
void startFlag_compete();
void compete_0();
void compete1_2();
void control_1();
void control_2();
void control_3();
void control_4();
void Serialprint();
void Serialprint2();
void setup_servo();

// =================================================================
// ⚙️ 【極限・ノータイム連射】遅延を完全にゼロにするパッキング送信関数
// =================================================================
void PackAndSendStationData() {
  sendData.header       = 99;              
  sendData.train1_speed = Send1 * 1.4;     
  sendData.fb_light3    = FB_Light;        
  sendData.train2_speed = Send2 * 4.5;     
  sendData.fb_light4    = FB_Light;         
  sendData.train3_speed = Send3;           
  sendData.data5_light  = FB_Light;         
  sendData.train4_speed = Send4 * 2;       
  sendData.data6_light  = FB_Light;         
  sendData.train1_poji  = Train1;          
  sendData.train2_poji  = Train2;          
  sendData.train3_poji  = Train3;          
  sendData.train4_poji  = Train4;          
  sendData.Data5_SIn    = Data5;           
  sendData.Data6_SOut   = Data6;           
  sendData.Data7_Sub    = Data7;           
  sendData.Data8_Main   = Data8;           
  sendData.ctr          = ctr;             
  sendData.startbutton  = startbutton;     
  sendData.data88       = 88;              
  sendData.voltage      = currentFeedbackVoltage; 

  unsigned long currentTime = millis();

  // 💡 【超即応化チューニング】
  bool isChanged = (sendData.train1_speed != lastSendData.train1_speed) ||
                   (sendData.train2_speed != lastSendData.train2_speed) ||
                   (sendData.train3_speed != lastSendData.train3_speed) ||
                   (sendData.train4_speed != lastSendData.train4_speed) ||
                   (sendData.Data5_SIn    != lastSendData.Data5_SIn)    ||
                   (sendData.Data6_SOut   != lastSendData.Data6_SOut)   ||
                   (sendData.voltage      != lastSendData.voltage);

  // 💡 列車2の速度がストップ(0)に変わった瞬間を自動検知してタイマースタート
  if ((sendData.train2_speed == 0) && (lastSendData.train2_speed > 0)) {
    stopSignalSentTime = currentTime;
    isWaitingForTrain2Ack = true;
  }

  if (isChanged || (currentTime - lastSendTime >= 30)) {
    esp_now_send(broadcastAddress, (uint8_t *)&sendData, sizeof(CombinedPayload));
    lastSendData = sendData;
    lastSendTime = currentTime;
  }
}

void setup(){
  setup_servo();
  Serial.println("start_button");
  startbutton = 1;
  Serial.begin(250000);   
  Serial2.begin(1000000, SERIAL_8N1, 16, 17);  

  pinMode(StartSW, INPUT_PULLUP);
  pinMode(servoInt, OUTPUT);
  pinMode(NANO_reset, OUTPUT);
  pinMode(EN_reset, OUTPUT);
  
  digitalWrite(NANO_reset, LOW);
  delay(500);
  digitalWrite(NANO_reset, HIGH);

  if(!SPIFFS.begin(true)){
    return;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = (uint8_t)0xff;
  }
  esp_now_add_peer(&slave);

  Serial.println("start_button1");
  while(digitalRead(StartSW) == HIGH){}
  delay(2000);
  startbutton = 0; 
  delay(2000); 
  Serial.println("start_button2");
  while(digitalRead(StartSW) == HIGH){}
  startbutton = 2;
  delay(2000); 

  Serial2.write("R");
  delay(100);
  Serial2.write("4");
  Serial2.write("Z");
  setup_servo();
}

void loop(){
  Receive(); // ①シリアル等の受信

  // ②速度制御計算
  if(compete == 1){
    control_1(); control_2(); control_3(); control_4();
  }
  if(compete == 0){
    compete_0();
  } 

  // ③手動デバッグ用割り込み
  if (Serial.available() > 0) {
    char input = Serial.read();
    switch(input) {
      case 's': Send1 = 0; break;
      case 'w': Send1 = 50; break;
      case 'e': Send2 = 50; break;
      case 'd': Send2 = 0; break;
    }
  }

  // ④極限送信関数を実行
  PackAndSendStationData(); 
  
  // ⑤周辺機器の制御
  in_Servo_Contlor();
  out_Servo_Contlor();
  sankaku();
  SendV();
  startFlag_compete();

  delay(20); 
}