#include <Arduino.h>
// 腳位定義
const int soundPin = D0; 

// 變數設定
unsigned long lastCheckTime = 0;
int soundCount = 0;
const int checkInterval = 2000; // 每 2 秒結算一次練習強度

void setup() {
  pinMode(soundPin, INPUT);
  Serial.begin(115200);
  Serial.println("===============================");
  Serial.println("吉他練習偵測系統：硬體測試模式");
  Serial.println("請開始彈奏吉他...");
  Serial.println("===============================");
}

void loop() {
  // 1. 即時偵測聲音觸發 (DO 在偵測到聲音時通常輸出 LOW)
  if (digitalRead(soundPin) == LOW) {
    soundCount++;
    // 在序列埠印出一個點，代表偵測到一個音符脈衝
    Serial.print("."); 
    delay(20); // 避開訊號抖動，不重複計數
  }

  // 2. 每隔一段時間 (checkInterval) 結算一次強度
  if (millis() - lastCheckTime >= checkInterval) {
    Serial.println(); // 換行
    Serial.print(">>> 過去 2 秒練習強度: ");
    Serial.print(soundCount);
    
    // 根據強度給予評價（你可以根據實測調整數字 5, 20）
    if (soundCount > 20) {
      Serial.println(" [ 🎸 搖滾中！ ]");
    } else if (soundCount > 5) {
      Serial.println(" [ 🎶 暖手中... ]");
    } else {
      Serial.println(" [ 🤫 安靜 ]");
    }

    soundCount = 0; // 重置計數
    lastCheckTime = millis();
  }
}
