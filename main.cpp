#include <Arduino.h>
// --- 硬體與邏輯設定 (固定不變，用 #define) ---
#define SOUND_PIN      D0
#define SOIL_PIN       A0
#define SOUND_CHECK_INTERVAL 5000     // 2秒結算一次
#define MOISTURE_CHECK_INTERVAL 600000 // 10分鐘結算一次
#define START_THRESHOLD 5       // 門檻：2秒內超過10次判定為彈奏中
// #define MAX_SILENCE    600000   // 10分鐘沒聲音就停止計時
#define MAX_SILENCE    10000   // 10s沒聲音就停止計時

// --- 系統運作狀態 (會改變，用變數) ---
int soundCount = 0;
int triggerCount = 0;
int playingTime = 0; 
bool isPlaying = false;         
unsigned long lastSoundTime = 0; // 紀錄最後一次聽到聲音的時間
unsigned long totalPlayingTime = 0; 
unsigned long lastCheckTime = 0;
unsigned long lastIntervalTime = 0;
unsigned long lastMoistureCheckTime = 0;
unsigned long now;

void volumeCheck(){
    // 1. 即時偵測聲音觸發 (DO 在偵測到聲音時通常輸出 LOW)
    if (digitalRead(SOUND_PIN) == LOW) {
      soundCount++;
      // 在序列埠印出一個點，代表偵測到一個音符脈衝
      Serial.print("."); 
      delay(20); // 避開訊號抖動，不重複計數
    }
    
    // 2. 每隔一段時間 (checkInterval) 結算一次強度
    if (millis() - lastCheckTime >= SOUND_CHECK_INTERVAL) {
      Serial.println(); // 換行
      Serial.print(">>> 過去 2 秒練習強度: ");
      Serial.print(soundCount);
      
      // 根據強度給予評價（你可以根據實測調整數字 5, 20）
      if (soundCount > 20) {
        Serial.println(" [ 🎸 搖滾中！ ]");
        triggerCount++;
      } else if (soundCount > 5) {
        Serial.println(" [ 🎶 暖手中... ]");
        triggerCount++;
      } else {
        Serial.println(" [ 🤫 安靜 ]");
      }
    
      soundCount = 0; // 重置計數
      lastCheckTime = millis();
    }
}

void playingCheck(){
    now = millis();
    // 2. 每 2 秒結算一次強度 (判斷是否正在練習)
    if (now - lastIntervalTime > SOUND_CHECK_INTERVAL){
        if (triggerCount >= START_THRESHOLD){
            if (!isPlaying){
                isPlaying = true;
                Serial.println("🎸 練習開始！");
            }
            totalPlayingTime += (now-lastIntervalTime);
            lastSoundTime = now;
        }
        triggerCount = 0;
        lastIntervalTime = now;
    }

    // 3. 判斷是否太久沒聲音 (判斷是否停止練習)
    // 注意：這裡是用「當前時間」減去「最後活動時間」
    if (isPlaying && (lastSoundTime - now > MAX_SILENCE) ){
        isPlaying = false;
        Serial.println("🤫 太久沒聲音了，練習停止。");
    }    

}

void soilCheck(){
    now = millis();
    if (lastMoistureCheckTime - now > MOISTURE_CHECK_INTERVAL){
        int moisture = analogRead(SOIL_PIN); //0-1024
        Serial.println(); // 換行
        Serial.print(">>> 過去 10m soil moisture: ");
        Serial.print(moisture);
        lastMoistureCheckTime = now; 
    } 
}
void setup() {
    pinMode(SOUND_PIN, INPUT);
    pinMode(SOIL_PIN, INPUT);
    Serial.begin(115200);
    Serial.println("===============================");
    Serial.println("吉他練習偵測系統：硬體測試模式");
    Serial.println("請開始彈奏吉他...");
    Serial.println("===============================");
}

void loop() {

    volumeCheck();
    playingCheck();
    //soilCheck();

}
