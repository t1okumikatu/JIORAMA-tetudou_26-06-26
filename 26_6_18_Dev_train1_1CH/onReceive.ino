void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  // 1. パケットが届いたら「!」を出す（これで電波の到達を確認）
  Serial.print("!"); 
  
  // 2. 送信元のMACアドレスを表示
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.printf(" (len:%d)\n", len);

  // 3. もし ID 99 が届いていたら
  if (len >= 1 && data[0] == 99) {
    Serial.println(" -> OK: ID 99 FOUND!");
    Send1 = data[1];
  }
}
/*
// ==========================================
// 受信コールバック：最新環境(Core 3.x)完全対応版
// ==========================================
void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  
  // 1. パケットが届いたこと自体を知らせるデバッグ表示
  // シリアルモニタに「.」が出れば、通信自体は成功しています
  Serial.print("#"); 

  // 2. Stationの仕様（ID:99 かつ 必要最低限の長さ）をチェック
  if (len >= 13 && data[0] == 99) {
    
    // volatile変数へ代入
    Send1  = data[1]; 
    Send2  = data[2];
    Send3  = data[3];
    Send4  = data[4]; 
    ctr    = data[5];
    Train1 = data[6];
    Train2 = data[7];
    Train3 = data[8];
    Train4 = data[9];
    Data5  = data[10];
    Data6  = data[11]; 
    Data7  = data[12];
    
    // 受信成功時に中身を表示（確認できたらコメントアウトしてOK）
    Serial.printf("\n[RCV] ID:%d, Send1:%d, Data7:%d\n", data[0], Send1, Data7);
    
    // 速度制御などの関数を呼び出す場合
    // forward(Send1);
  }
}*/