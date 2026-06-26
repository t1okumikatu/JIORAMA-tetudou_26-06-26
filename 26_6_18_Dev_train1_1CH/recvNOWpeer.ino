#include <esp_wifi.h>  // ← これを一番上に追加！
void recvNowpeer(){
  // ★ 送信側と同じ ch1 に固定
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());
delay(3000);
  // ESP-NOW 初期化
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  // 受信コールバック登録
  esp_now_register_recv_cb(onReceive);//ESP-NOWの受信処理を有効化して、「受信待ち状態になった」ことを示すコードです。
  Serial.println("ESP-NOW Receiver Ready");
  delay(3000);
}