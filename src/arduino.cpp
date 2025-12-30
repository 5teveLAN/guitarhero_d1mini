#include <Arduino.h>

const int AUDIO_PIN = A0;
const int SIG_OUT = 2; // 接到 D1 Mini 的 D1

int baseline = 0;
int sensitivity = 3; // 1-2格音量建議設為 3

void setup() {
  Serial.begin(115200);
  pinMode(SIG_OUT, OUTPUT);
  
  // 校準環境音
  long sum = 0;
  for(int i=0; i<50; i++) { sum += analogRead(AUDIO_PIN); delay(10); }
  baseline = sum / 50;
  Serial.println("Uno 聲音偵測啟動...");
}

void loop() {
  int maxV = 0, minV = 1024;
  unsigned long start = millis();
  
  // 捕捉 50ms 內的震幅 (Peak-to-Peak)
  while(millis() - start < 50) {
    int v = analogRead(AUDIO_PIN);
    if(v > maxV) maxV = v; if(v < minV) minV = v;
  }

  // 只要有波動，就輸出 HIGH 給 D1 Mini
  if((maxV - minV) > sensitivity) {
    digitalWrite(SIG_OUT, HIGH);
    Serial.println("彈奏中...");
  } else {
    digitalWrite(SIG_OUT, LOW);
  }
}
