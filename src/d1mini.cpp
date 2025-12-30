#include <Arduino.h>

const int SOIL_DIGITAL_PIN = D5; // D0 接在 D5

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_DIGITAL_PIN, INPUT); 
  Serial.println("--- 土壤數位訊號 (D0) 測試開始 ---");
}

void loop() {
  // 讀取數位訊號 (0 或 1)
  int status = digitalRead(SOIL_DIGITAL_PIN);

  if (status == LOW) {
    // 通常低電位代表「偵測到水/濕度夠」
    Serial.println("目前狀態: [LOW] -> 濕潤 (LED 理論上要亮)");
  } else {
    // 通常高電位代表「太乾」
    Serial.println("目前狀態: [HIGH] -> 乾燥");
  }

  // 同時印出類比值參考，確認感測器到底有沒有反應
  Serial.print("類比參考值(A0): ");
  Serial.println(analogRead(A0));
  
  Serial.println("-------------------------");
  delay(500); // 每半秒偵測一次
}
