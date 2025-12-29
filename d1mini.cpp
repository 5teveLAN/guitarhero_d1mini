// ================= 設定區 (請修改這裡) =================

// 1. Blynk 設定 (從 Blynk Console 複製)
#define BLYNK_TEMPLATE_ID "TMPL6xK08movF"
#define BLYNK_TEMPLATE_NAME "Guitar"
#define BLYNK_AUTH_TOKEN "Pn3988JaNtRzC_j-DXQcaasgys8wFm3y"

// 2. WiFi 設定
char ssid[] = "你的WiFi名稱";
char pass[] = "你的WiFi密碼";

// 3. Discord Webhook 設定
const char* discord_webhook = "https://discord.com/api/webhooks/1452962936132861974/LRQemWICbIVswBpI40VlkJIJHmkWVs2s4Ip48PLO3DZwBhvRLHn4qxtyjVfDnAAO4m5V";

// 4. ThingSpeak 設定
unsigned long myChannelNumber = 3215158; // 你的 Channel ID
const char * myWriteAPIKey = "4UTH2I16EWW9CJUA";

// ======================================================

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "ThingSpeak.h"

// --- 硬體接腳定義 (D1 Mini) ---
const int SOUND_PIN = D1; // 聲音感測器 (接 Digital Out)
const int RELAY_PIN = D2; // 繼電器
const int SOIL_PIN = A0;  // 土壤感測器 (接 Analog Out)

// --- 全域變數 ---
int targetTime = 30;         // 目標時間 (Blynk V0 控制)
int practiceTime = 0;        // 累積練習秒數
int soilValue = 0;           // 土壤數值
bool isPumping = false;      // 馬達狀態

// --- 計時器變數 (取代 delay) ---
unsigned long lastSecondTimer = 0;
unsigned long lastThingSpeakTimer = 0;

WiFiClient  client; // 給 ThingSpeak 用

// --- Blynk 同步函式 ---
BLYNK_WRITE(V0) {
  targetTime = param.asInt();
  Serial.print("目標時間更新為: "); Serial.println(targetTime);
}

// --- Discord 發送函式 ---
void sendDiscord(String content) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // 忽略憑證檢查
    HTTPClient http;
    http.begin(secureClient, discord_webhook);
    http.addHeader("Content-Type", "application/json");
    
    // 建立 JSON 格式
    String payload = "{\"content\": \"" + content + "\"}";
    
    int httpCode = http.POST(payload);
    if (httpCode > 0) Serial.println("Discord 發送成功");
    else Serial.println("Discord 發送失敗");
    
    http.end();
  }
}

// --- ThingSpeak 上傳函式 ---
void uploadToThingSpeak() {
  ThingSpeak.setField(1, soilValue);            // Field 1: 濕度
  ThingSpeak.setField(2, practiceTime / 60);    // Field 2: 練習分鐘數
  
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200) Serial.println("ThingSpeak 上傳成功");
  else Serial.println("ThingSpeak 上傳失敗: " + String(x));
}

void setup() {
  Serial.begin(115200);
  
  // 設定接腳模式
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SOUND_PIN, INPUT);
  digitalWrite(RELAY_PIN, LOW); // 預設關閉 (若繼電器是低觸發，請改 HIGH)

  // 連線 Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Blynk.syncVirtual(V0); // 同步雲端設定
  
  // 初始化 ThingSpeak
  ThingSpeak.begin(client);
  
  sendDiscord("吉他英雄系統 (D1 Mini版) 已上線！");
}

void loop() {
  Blynk.run(); // 保持 Blynk 連線
  
  unsigned long currentMillis = millis();

  // --- 任務 A: 每 1 秒執行一次 (處理感測器與邏輯) ---
  if (currentMillis - lastSecondTimer > 1000) {
    lastSecondTimer = currentMillis;
    
    // 1. 讀取數據
    soilValue = analogRead(SOIL_PIN);
    int isSound = digitalRead(SOUND_PIN); // 讀取 0 或 1
    
    // 2. 回傳 Blynk (即時監控)
    Blynk.virtualWrite(V1, soilValue);
    
    // 3. 判斷聲音並計時
    // 注意：有些感測器有聲音是LOW，有些是HIGH，請依實際情況調整
    if (isSound == HIGH) { 
      practiceTime++; // 累加 1 秒
      Serial.println("偵測到練習聲...");
      
      // 更新 Blynk 顯示 (V2)
      Blynk.virtualWrite(V2, practiceTime / 60); 
    }
    
    // 4. 判斷是否達成目標
    // 將 targetTime (分) 轉為 (秒) 來比較
    if (practiceTime >= (targetTime * 60) && !isPumping) {
      isPumping = true;
      Serial.println("目標達成！澆水！");
      sendDiscord("🎉 恭喜！今日練習目標達成，正在為植物澆水！");
      
      digitalWrite(RELAY_PIN, HIGH); // 開馬達
      
      // 上傳達成時的數據
      uploadToThingSpeak(); 
      
      // 澆水 3 秒 (這裡可以用 delay 因為動作很短)
      delay(3000); 
      
      digitalWrite(RELAY_PIN, LOW);  // 關馬達
      isPumping = false;
      
      // 選擇性：達成後是否重置時間？
      // practiceTime = 0; 
    }
  }

  // --- 任務 B: 每 20 秒執行一次 (上傳 ThingSpeak) ---
  if (currentMillis - lastThingSpeakTimer > 20000) {
    lastThingSpeakTimer = currentMillis;
    uploadToThingSpeak();
  }
}
