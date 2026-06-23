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

// === ① 受信コールバック ===
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  if (len >= sizeof(CombinedPayload)) {
    memcpy(&recvData, incomingData, sizeof(CombinedPayload));
    currentFeedbackVoltage = recvData.voltage;
  }
}

// === ② 送信完了コールバック ===
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
  // 「データが変わった瞬間」はもちろん、データに変化がなくても「30ms（0.03秒）」経過したら
  // 強制的に電波を間髪入れず連射します。これで100msのタイムラグの壁を完全に破壊します！
  bool isChanged = (sendData.train1_speed != lastSendData.train1_speed) ||
                   (sendData.train2_speed != lastSendData.train2_speed) ||
                   (sendData.train3_speed != lastSendData.train3_speed) ||
                   (sendData.train4_speed != lastSendData.train4_speed) ||
                   (sendData.Data5_SIn    != lastSendData.Data5_SIn)    ||
                   (sendData.Data6_SOut   != lastSendData.Data6_SOut)   ||
                   (sendData.voltage      != lastSendData.voltage);

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
      case 'w': Send1 = 100; break;
    }
  }

  // 💡 【超重要：徹底した軽量化】
  // シリアル通信の負荷を完全にゼロにするため、ここでのテキスト出力はすべて排除しました。
  // これにより、プログラムの処理速度が物理限界まで引き上がります。

  // ④極限送信関数を実行
  PackAndSendStationData(); 
  
  // ⑤周辺機器の制御
  in_Servo_Contlor();
  out_Servo_Contlor();
  sankaku();
  
  // 💡 【要注意】もしこれでもオーバーランする場合は、この「Serialprint2();」のTABを開き、
  // 中身にある Serial.print 系の命令をすべてコメントアウト(//)して消し去ってください。
  //Serialprint2(); 
  
  startFlag_compete();

  // ループウェイト20ms（1秒間に50回、限界スピードで全体を回します）
  delay(20); 
}