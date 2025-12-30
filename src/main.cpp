// ================= 設定區 (請修改這裡) =================

// 1. Blynk 設定
#define BLYNK_TEMPLATE_ID "TMPL6bea9_UMu"
#define BLYNK_TEMPLATE_NAME "Guitar"
#define BLYNK_AUTH_TOKEN "ZtsHsg7cr8Hb8_YTUH_W1yek9gt12NSR"
// 2. WiFi 設定
char ssid[] = "Ggd_ap";
char pass[] = "04282009";

// 3. Discord Webhook 設定
const char* discord_webhook = "https://discord.com/api/webhooks/1455384795373895953/OpMgsvwAaIl7OpzyXG5pWU53yUOmOUEQLCnbV_pSvshI5K8TYGQLVzLGDqqvBH3RoJiH";

// 4. ThingSpeak 設定
//unsigned long myChannelNumber = 3215158;
//const char * myWriteAPIKey = "4UTH2I16EWW9CJUA";

// ======================================================

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "ThingSpeak.h"
#include <WidgetRTC.h> // 加入 Blynk 的時間函式庫

// --- 硬體接腳定義 (D1 Mini) ---
const int AUDIO_PIN = A0;			// 3.5mm 音訊接 Analog
const int RELAY_PIN = D2;			// 繼電器
const int SOIL_DIGITAL_PIN = D5;	// 土壤感測器 D0 接 D5

// --- 全域變數 ---
int targetTime = 30;
int practiceTime = 0;
bool isPumping = false;
bool hasWateredToday = false;		// 一天限一次旗標
bool lastSoilStatus = HIGH;			// 記錄上次土壤狀態 (HIGH為乾)

unsigned long lastSecondTimer = 0;
unsigned long lastThingSpeakTimer = 0;

WiFiClient client;

WidgetRTC rtc; // 建立 RTC 物件
int last_day = -1; // 用來記錄上一次檢查的天數

// --- 函式：判斷是否在練習 (A0音訊處理) ---
bool checkPractice() {
	int maxV = 0, minV = 1024;
	unsigned long start = millis();
	// 採樣 50ms 捕捉音訊震幅
	while (millis() - start < 50) {
		int v = analogRead(AUDIO_PIN);
		if (v > maxV) maxV = v;
		if (v < minV) minV = v;
	}
	int amplitude = maxV - minV;
	int sensitivity = 5; // 靈敏度調整
	return (amplitude > sensitivity);
}

// --- Blynk 同步函式 ---
BLYNK_WRITE(V0) {
	targetTime = param.asInt();
	Serial.print("目標時間更新為: "); Serial.println(targetTime);
}
// 建立一個檢查時間並重置的函式
void checkTimeReset() {
	// 獲取目前日期中的「天」 (1-31)
	int current_day = day(); 

	// 如果偵測到換天了（例如從 30 號變成 31 號）
	if (current_day != last_day) {
		Serial.println("偵測到新的一天，重置澆水權限與練習時間。");
		hasWateredToday = false; // 恢復今日澆水權限
		practiceTime = 0;        // 練習秒數歸零 (看你是否要每天重算)
		last_day = current_day;  // 更新日期記錄
		
		Blynk.virtualWrite(V2, 0); // 更新手機上的練習時間顯示
	}
}

// --- Discord 發送函式 ---
void sendDiscord(String content) {
	if (WiFi.status() == WL_CONNECTED) {
		WiFiClientSecure secureClient;
		secureClient.setInsecure();
		HTTPClient http;
		http.begin(secureClient, discord_webhook);
		http.addHeader("Content-Type", "application/json");
		String payload = "{\"content\": \"" + content + "\"}";
		//int httpCode = http.POST(payload);
		http.end();
	}
}

// --- ThingSpeak 上傳函式 ---
/*
void uploadToThingSpeak() {
	// 因為改用數位，這裡上傳 0 或 100 代表濕度狀態
	int soilStatus = digitalRead(SOIL_DIGITAL_PIN);
	ThingSpeak.setField(1, (soilStatus == LOW) ? 100 : 0);
	ThingSpeak.setField(2, practiceTime / 60);
	ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}
*/

void setup() {
	Serial.begin(115200);
	pinMode(RELAY_PIN, OUTPUT);
	pinMode(SOIL_DIGITAL_PIN, INPUT);
	digitalWrite(RELAY_PIN, LOW);

	Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
	Blynk.syncVirtual(V0);
    // 在 Blynk 連線後啟動 RTC
	setSyncInterval(10 * 60); // 每 10 分鐘同步一次時間
	ThingSpeak.begin(client);
	
	sendDiscord("🎸 吉他練習監控系統已啟動！");
}

void loop() {
	Blynk.run();
	unsigned long currentMillis = millis();

	// --- 任務 A: 每 1 秒執行一次 ---
	if (currentMillis - lastSecondTimer > 1000) {
		lastSecondTimer = currentMillis;
	    // 0. 先檢查是否需要跨日重置
		checkTimeReset();	
		// 1. 讀取數據
		bool isPracticing = checkPractice();			// 判斷音訊
		int soilStatus = digitalRead(SOIL_DIGITAL_PIN); // HIGH=乾, LOW=濕
		
		// 2. 防弊偵測：時間還沒到，但土突然變濕了
		if (practiceTime < (targetTime * 60) && lastSoilStatus == HIGH && soilStatus == LOW) {
			Serial.println("偵測到偷澆水！");
			sendDiscord("⚠️ 偵測到土壤濕度異常！練習目標尚未達成，請勿偷澆水！");
		}
		lastSoilStatus = soilStatus;
		
		// 3. 回傳 Blynk (V1 顯示狀態)
		Blynk.virtualWrite(V1, (soilStatus == LOW) ? 1 : 0);
		
		// 4. 判斷練習計時
		if (isPracticing) { 
			practiceTime++;
			Blynk.virtualWrite(V2, practiceTime / 60); 
			Serial.print("練習中... 累計秒數: "); Serial.println(practiceTime);
		}
		
		// 5. 判斷是否自動澆水 (達成目標 && 土壤乾燥 && 今日未澆)
		if (practiceTime >= (targetTime * 60) && !hasWateredToday && !isPumping) {
			if (soilStatus == HIGH) { // 只有土乾的時候才啟動
				isPumping = true;
				Serial.println("目標達成！執行自動澆水。");
				sendDiscord("🎉 今日練習目標已達成！自動噴水系統啟動。");
				
				digitalWrite(RELAY_PIN, HIGH);
				//uploadToThingSpeak(); // 達成時立刻記錄
				delay(3000); 
				digitalWrite(RELAY_PIN, LOW);
				
				isPumping = false;
				hasWateredToday = true; // 鎖定今日任務
			}
		}
	}

	// --- 任務 B: 每 20 秒上傳數據 ---
    /*
	if (currentMillis - lastThingSpeakTimer > 20000) {
		lastThingSpeakTimer = currentMillis;
		uploadToThingSpeak();
	}
    */
}
