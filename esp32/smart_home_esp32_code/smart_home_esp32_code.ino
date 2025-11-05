#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>
#include "credentials.h"

//==================== KHAI BÁO PHẦN CỨNG ====================//
// Cảm biến
const int PIN_MQ2          = 34;
const int PIN_REED         = 27;
const int PIN_IR_REFLECT   = 14;
// Relay điều khiển
const int PIN_RELAY_LIGHT  = 26;
const int PIN_RELAY_FAN    = 25;
// Servo mô phỏng cửa
const int PIN_SERVO        = 15;
// Còi cảnh báo
const int PIN_BUZZER       = 13;
// RFID RC522
const int PIN_RC522_SS     = 5;
const int PIN_RC522_RST    = 4;

// SPI default ESP32: SCK=18, MISO=19, MOSI=23
#define MQ2_SAMPLE_COUNT    10
#define MQ2_ALARM_THRESH    1800
#define ALARM_BUZZER_MS     300
#define DOOR_AUTO_CLOSE_MS  5000

// Danh sách UID hợp lệ
// const char* AUTH_UIDS[] = { "B03B2258" };
// const size_t AUTH_UIDS_COUNT = sizeof(AUTH_UIDS)/sizeof(AUTH_UIDS[0]);

//==================== ĐỐI TƯỢNG ====================//
WebServer server(80);
MFRC522 mfrc522(PIN_RC522_SS, PIN_RC522_RST);
Servo doorServo;

bool relayLightOn = false;
bool relayFanOn   = false;
bool alarmActive  = false;
bool doorOpen     = false;
unsigned long lastDoorActionMs = 0;
unsigned long lastAlarmBeepMs  = 0;
unsigned long alarmStartTime   = 0;

//==================== TIỆN ÍCH ====================//
String uidToString(MFRC522::Uid *uid) {
  String s = "";
  for (byte i=0;i<uid->size;i++){
    if (uid->uidByte[i] < 0x10) s += "0";
    s += String(uid->uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

// bool isAuthorizedUID(const String& uidHex) {
//   for (size_t i=0;i<AUTH_UIDS_COUNT;i++) {
//     if (uidHex == AUTH_UIDS[i]) return true;
//   }
//   return false;
// }

// #include <HTTPClient.h>

bool checkUIDFromServer(String uid) {
  HTTPClient http;
  String url = "http://172.20.10.2:3000/api/rfid/" + uid;  // ⚠️ Thay bằng IP máy chạy Node.js
  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    if (payload.indexOf("\"authorized\":true") >= 0) {
      Serial.println("[RFID] ✅ UID hợp lệ từ server");
      http.end();
      return true;
    }
  }
  Serial.println("[RFID] ❌ UID không hợp lệ (server từ chối)");
  http.end();
  return false;
}


int readMQ2Averaged() {
  long sum = 0;
  for (int i=0;i<MQ2_SAMPLE_COUNT;i++){
    sum += analogRead(PIN_MQ2);
    delay(2);
  }
  return (int)(sum / MQ2_SAMPLE_COUNT);
}

void setRelay(int pin, bool on) {
  // ✅ Active-HIGH logic để khớp giao diện web
  digitalWrite(pin, on ? HIGH : LOW);
  Serial.printf("[RELAY] Pin %d -> %s\n", pin, on ? "BẬT" : "TẮT");
}

void setBuzzer(bool on) { digitalWrite(PIN_BUZZER, on ? HIGH : LOW); }

void openDoor() {
  doorServo.write(90);
  doorOpen = true;
  lastDoorActionMs = millis();
  Serial.println("[DOOR] Mở cửa");
}

void closeDoor() {
  doorServo.write(0);
  doorOpen = false;
  Serial.println("[DOOR] Đóng cửa");
}

//==================== HANDLER ====================//
void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
  addCORS();
  server.send(200, "application/json", "{\"service\":\"ESP32 Smart Home API active\"}");
}

void handleStatus() {
  int mq2 = readMQ2Averaged();
  bool reed = digitalRead(PIN_REED) == HIGH;
  bool ir   = digitalRead(PIN_IR_REFLECT) == LOW;
  addCORS();
  String json = "{";
  json += "\"mq2\":" + String(mq2) + ",";
  json += "\"reed\":" + String(reed ? 1:0) + ",";
  json += "\"ir\":" + String(ir ? 1:0) + ",";
  json += "\"alarm\":" + String(alarmActive ? 1:0) + ",";
  json += "\"light\":" + String(relayLightOn ? 1:0) + ",";
  json += "\"fan\":" + String(relayFanOn ? 1:0) + ",";
  json += "\"door\":" + String(doorOpen ? 1:0);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRelay() {
  addCORS();
  if (!server.hasArg("ch")) { server.send(400, "application/json", "{\"err\":\"missing ch\"}"); return; }
  String ch = server.arg("ch");
  if (ch == "light") {
    relayLightOn = !relayLightOn;
    setRelay(PIN_RELAY_LIGHT, relayLightOn);
  } else if (ch == "fan") {
    relayFanOn = !relayFanOn;
    setRelay(PIN_RELAY_FAN, relayFanOn);
  } else {
    server.send(400, "application/json", "{\"err\":\"bad ch\"}"); return;
  }
  Serial.printf("[WEB] Lệnh điều khiển: %s\n", ch.c_str());
  handleStatus();
}

void handleServo() {
  addCORS();
  int angle = server.hasArg("angle") ? server.arg("angle").toInt() : 0;
  angle = constrain(angle, 0, 180);
  doorServo.write(angle);
  doorOpen = (angle >= 45);
  Serial.printf("[WEB] Servo cửa -> %d°\n", angle);
  if (doorOpen) lastDoorActionMs = millis();
  handleStatus();
}

void handleResetAlarm() {
  addCORS();
  alarmActive = false;
  setBuzzer(false);
  Serial.println("[WEB] Reset cảnh báo");
  handleStatus();
}

void handleVoice() {
  addCORS();
  if (!server.hasArg("cmd")) { server.send(400, "application/json", "{\"err\":\"missing cmd\"}"); return; }
  String cmd = server.arg("cmd");
  cmd.toLowerCase();
  Serial.printf("[VOICE] Nhận lệnh: %s\n", cmd.c_str());

  if (cmd.indexOf("bật đèn") >= 0) { relayLightOn = true; setRelay(PIN_RELAY_LIGHT, true); }
  else if (cmd.indexOf("tắt đèn") >= 0) { relayLightOn = false; setRelay(PIN_RELAY_LIGHT, false); }
  else if (cmd.indexOf("bật quạt") >= 0) { relayFanOn = true; setRelay(PIN_RELAY_FAN, true); }
  else if (cmd.indexOf("tắt quạt") >= 0) { relayFanOn = false; setRelay(PIN_RELAY_FAN, false); }
  else if (cmd.indexOf("mở cửa") >= 0) { openDoor(); }
  else if (cmd.indexOf("đóng cửa") >= 0) { closeDoor(); }
  else Serial.println("[VOICE] ⚠️ Lệnh không hợp lệ");

  handleStatus();
}

//==================== SETUP ====================//
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_REED, INPUT_PULLUP);
  pinMode(PIN_IR_REFLECT, INPUT_PULLUP);
  pinMode(PIN_RELAY_LIGHT, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  setRelay(PIN_RELAY_LIGHT, false);
  setRelay(PIN_RELAY_FAN, false);
  setBuzzer(false);

  doorServo.attach(PIN_SERVO);
  closeDoor();

  SPI.begin();
  mfrc522.PCD_Init(PIN_RC522_SS, PIN_RC522_RST);
  Serial.println("🟢 RC522 sẵn sàng.");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("🔌 Kết nối WiFi");
  int retry=0;
  while (WiFi.status() != WL_CONNECTED && retry < 60) {
    delay(500); Serial.print("."); retry++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✅ WiFi OK. IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("⚠️ WiFi thất bại, khởi động AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_SMART_HOME", "12345678");
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  }

  // Khai báo route
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/relay", HTTP_POST, handleRelay);
  server.on("/api/servo", HTTP_POST, handleServo);
  server.on("/api/reset_alarm", HTTP_POST, handleResetAlarm);
  server.on("/api/voice", HTTP_POST, handleVoice);
  server.begin();
  Serial.println("🌐 HTTP server started");
}

//==================== LOOP ====================//
void loop() {
  server.handleClient();

  static unsigned long lastSenseMs = 0;
  unsigned long now = millis();

  // Cảm biến và cảnh báo
  if (now - lastSenseMs >= 200) {
    lastSenseMs = now;
    int mq2 = readMQ2Averaged();
    bool reedClosed = (digitalRead(PIN_REED) == HIGH);
    bool irDetected  = (digitalRead(PIN_IR_REFLECT) == LOW);

    if (mq2 >= MQ2_ALARM_THRESH || !reedClosed || irDetected) {
      alarmActive = true;
      Serial.println("[ALARM] ⚠️ Kích hoạt báo động!");
    }

    if (alarmActive) {
      if (alarmStartTime == 0) alarmStartTime = now;
      if (now - lastAlarmBeepMs >= 600) {
        lastAlarmBeepMs = now;
        setBuzzer(true);
        delay(ALARM_BUZZER_MS);
        setBuzzer(false);
      }
      if (now - alarmStartTime >= 5000) {
        alarmActive = false;
        alarmStartTime = 0;
        setBuzzer(false);
        Serial.println("[ALARM] 🟢 Báo động tự tắt sau 5s");
      }
    } else alarmStartTime = 0;
  }

  // Đóng cửa tự động
  if (doorOpen && (millis() - lastDoorActionMs >= DOOR_AUTO_CLOSE_MS)) {
    closeDoor();
    Serial.println("[DOOR] 🔒 Tự động đóng sau 5 giây");
  }

  // RFID quét thẻ
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uidHex = uidToString(&mfrc522.uid);
    Serial.printf("[RFID] Quét UID: %s\n", uidHex.c_str());
    bool ok = checkUIDFromServer(uidHex);
    if (ok) {
      openDoor();
      alarmActive = false;
      setBuzzer(false);
      Serial.println("[RFID] ✅ Thẻ hợp lệ -> Mở cửa");
    } else {
      alarmActive = true;
      Serial.println("[RFID] ❌ Thẻ KHÔNG hợp lệ -> Báo động");
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}
