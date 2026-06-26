#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "esp_wifi.h"

const char* ssid = "Train5_Monitor";
const char* password = "password123";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// 💡 各データを安全に独立キープしておくためのグローバル変数
float lastKnownVoltage = 0.0;
float lastKnownDelay = 0.0; // 🎯 ここに最新の遅延ms（77または66由来）が保持され続けます

struct __attribute__((packed)) CombinedPayload {
  uint8_t header;       // data[0]  (99)
  uint8_t train1_speed; // data[1]
  uint8_t fb_light3;    // data[2]
  uint8_t train2_speed; // data[3]
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
  uint8_t data88;       // data[19]  (88:電圧, 77:ステーション往復遅延, 66:列車2内部遅延)
  
  float voltage;        // ステーションまたは各列車から届く数値
};

CombinedPayload incomingPack; 
CombinedPayload stationData; 

// =================================================================
// 📱 スマホ用 HTML / JavaScript 画面データ（変更なし・完全対応）
// =================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Train5 Monitor 遅延実測版</title>
    <style>
        body { font-family: Arial, sans-serif; background: #222; color: #fff; margin: 20px; }
        h1 { text-align: center; color: #00ffcc; font-size: 24px; }
        .container { max-width: 500px; margin: 0 auto; background: #333; padding: 15px; border-radius: 10px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
        .data-row { display: flex; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid #444; font-size: 16px; }
        .label { color: #aaa; }
        .value { font-weight: bold; color: #ffeb3b; font-family: monospace; }
        .space-above { margin-top: 15px; }
        
        .voltage-box { background: #111; border: 1px solid #00ffcc; padding: 12px; border-radius: 8px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; }
        .voltage-label { color: #00ffcc; font-weight: bold; font-size: 16px; }
        .voltage-value { font-size: 22px; color: #00ffcc; font-weight: bold; font-family: monospace; }
        
        .delay-box { background: #111; border: 1px solid #ff9800; padding: 12px; border-radius: 8px; margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center; }
        .delay-label { color: #ff9800; font-weight: bold; font-size: 16px; }
        .delay-value { font-size: 22px; color: #ff9800; font-weight: bold; font-family: monospace; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Train5 運行モニター</h1>
        
        <div class="voltage-box">
            <span class="voltage-label">🔋 トレイン2 バッテリー電圧:</span>
            <span class="voltage-value"><span id="voltage_val">0.00</span> V</span>
        </div>

        <div class="delay-box">
            <span class="delay-label">⏳ 最終ストップ時の片道飛行時間:</span>
            <span class="delay-value"><span id="delay_val">0.0</span> ms</span>
        </div>

        <div class="data-row"><span class="label">Train Spd1:</span><span class="value" id="d1">-</span></div>
        <div class="data-row"><span class="label">Train Spd2:</span><span class="value" id="d3">-</span></div>
        <div class="data-row"><span class="label">Train Spd3:</span><span class="value" id="d5">-</span></div>
        <div class="data-row"><span class="label">Train Spd4:</span><span class="value" id="d7">-</span></div>
        
        <div class="data-row space-above"><span class="label">Train1 / 2 位置:</span><span class="value"><span id="d9">-</span> / <span id="d10">-</span></span></div>
        <div class="data-row"><span class="label">Train3 / 4 位置:</span><span class="value"><span id="d11">-</span> / <span id="d12">-</span></span></div>
        
        <div class="data-row"><span class="label">SIn / SOut:</span><span class="value" id="custom_status">- / -</span></div>
        
        <div class="data-row space-above"><span class="label">Counter / StartBtn:</span><span class="value"><span id="d17">-</span> / <span id="d18">-</span></span></div>
    </div>

    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onmessage = onMessage;
            websocket.onclose = function() { setTimeout(initWebSocket, 2000); };
        }
        function onMessage(event) {
            var data = event.data.trim().split(',');
            
            if(data.length >= 22) { 
                document.getElementById('d1').innerText  = data[1];
                document.getElementById('d3').innerText  = data[3];
                document.getElementById('d5').innerText  = data[5];
                document.getElementById('d7').innerText  = data[7];
                document.getElementById('d9').innerText  = data[9];
                document.getElementById('d10').innerText = data[10];
                document.getElementById('d11').innerText = data[11];
                document.getElementById('d12').innerText = data[12];
                document.getElementById('d17').innerText = data[17];
                document.getElementById('d18').innerText = data[18];
                
                var sinVal = data[13];
                var soutVal = data[14];
                if (sinVal == "1") sinVal = "SUB"; else if (sinVal == "2" || sinVal == "0") sinVal = "MAIN";
                if (soutVal == "1") soutVal = "SUB"; else if (soutVal == "2" || soutVal == "0") soutVal = "MAIN";

                var statusEl = document.getElementById('custom_status');
                if (statusEl) statusEl.innerText = sinVal + " / " + soutVal;
                
                // 21番目の電圧データを表示
                var rawVolt = parseInt(data[20]);
                if (rawVolt > 0) {
                    var voltVal = rawVolt / 50.0;
                    var voltEl = document.getElementById('voltage_val');
                    if (voltEl) voltEl.innerText = voltVal.toFixed(2);
                }

                // 22番目の遅延時間(ms)データを表示
                var delayVal = parseFloat(data[21]);
                var delayEl = document.getElementById('delay_val');
                if (delayEl) delayEl.innerText = delayVal.toFixed(1);
            }
        }
        window.addEventListener('load', initWebSocket);
    </script>
</body>
</html>
)rawliteral";

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {}

// === 🎯 ESP-NOW 受信コールバック（66/77/88 完全対応マルチセパレータ版） ===
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  
  if (len >= sizeof(CombinedPayload)) {
    memcpy(&incomingPack, incomingData, sizeof(CombinedPayload));
    
    // 💡 列車・ステーション・電圧すべての電波(99)を受け入れるように拡大
    if (incomingPack.header == 99 && 
       (incomingPack.data88 == 88 || incomingPack.data88 == 77 || incomingPack.data88 == 66)) {
      
      // 基本となる構造体情報を上書き同期（位置情報やカウンタが飛んでも連動するため）
      stationData = incomingPack;

      if (stationData.data88 == 77 || stationData.data88 == 66) {
        // ⏳ 77（ステーション計測遅延）または 66（列車内部の遅延）の時は、遅延msをガッチリ更新
        lastKnownDelay = stationData.voltage;
      } else if (stationData.data88 == 88) {
        // 🔋 通常データの時は、いつも通りバッテリー電圧として処理
        if (stationData.voltage > 0.01) {
          lastKnownVoltage = stationData.voltage;
        }
      }

      // スマホ画面へCSV全同期データを構築して送信
      String csvStr = "";
      for(int i = 0; i < 20; i++) {
        csvStr += String(((uint8_t*)&stationData)[i]) + ",";
      }
      
      // 21番目の要素：キープされている最新の「バッテリー電圧値」をエンコード
      uint8_t packedVolt = (uint8_t)(lastKnownVoltage * 50.0 + 0.5);
      csvStr += String(packedVolt) + ","; 

      // 22番目の要素：キープされている最新の「飛行時間/遅延時間ms」をそのまま結合！
      csvStr += String(lastKnownDelay, 1);

      ws.textAll(csvStr); 
    } 
  }
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password, 1); 
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  server.begin();
  Serial.println("Train5 Monitor（66/77遅延マルチホールド完全版）稼働しました！");
}

void loop() {
  ws.cleanupClients();
  delay(50);
}