/*************************************************************
  BẮT BUỘC ĐẶT 3 DÒNG NÀY TRÊN CÙNG
 *************************************************************/
#define BLYNK_TEMPLATE_ID "TMPL6HzyEGgpW"
#define BLYNK_TEMPLATE_NAME "Dieu khien led"
#define BLYNK_AUTH_TOKEN "0bjWpoKBVGuLzk0lSo3r3CMvrLHYEVhm"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <NewPing.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// --- Cấu hình Pin ---
#define TRIGGER_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200
#define ONE_WIRE_BUS 14
#define TURBIDITY_PIN 34

const int relaySoLanh = 25; // V13
const int relayBom = 26;    // V14
const int relayDen = 27;    // V15
const int servoPin = 19;

// --- Khởi tạo đối tượng ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo myServo;
WidgetRTC rtc;
BlynkTimer timer;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// --- Biến hệ thống ---
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "TANG 3";
char pass[] = "123456789";

int tankHeight = 0;
int waterLevel = 0;
float currentTemp = 0.0;
float currentNTU = 0.0;
int schedHours[3] = { -1, -1, -1 }, schedMins[3] = { -1, -1, -1 };
bool alreadyFired[3] = { false, false, false };

bool lastPumpState = false;
bool lastFanState = false;

unsigned long pumpManualTimer = 0;
unsigned long fanManualTimer = 0;
const long overrideTime = 30000; 

// --- Khai báo mẫu hàm ---
void checkSchedule();
void processSystem();
void updateSchedule(int index, BlynkParam param);

/***************************************************
 * PHẦN AI: HỒI QUY TUYẾN TÍNH
 ***************************************************/
float w_pump, b_pump, w_fan, b_fan;
float train_x[15][2] = { {0.05,0.20}, {0.10,0.25}, {0.15,0.30}, {0.20,0.35}, {0.30,0.40}, {0.35,0.50}, {0.40,0.55}, {0.50,0.60}, {0.60,0.65}, {0.70,0.70}, {0.75,0.75}, {0.80,0.80}, {0.85,0.85}, {0.90,0.90}, {0.95,0.95} };
float train_y[15][2] = { {1,0}, {1,0}, {1,0}, {1,0}, {1,0}, {1,0}, {1,0}, {1,1}, {0,1}, {0,1}, {0,1}, {0,1}, {0,1}, {0,1}, {0,1} };

void train_models() {
  w_pump = random(-100, 100) / 100.0; b_pump = random(-100, 100) / 100.0;
  w_fan = random(-100, 100) / 100.0;  b_fan = random(-100, 100) / 100.0;
  float lr0 = 1.5, decay = 0.04;

  for (int e = 0; e < 100; e++) {
    float lr = lr0 / (1.0 + decay * e);
    float dwp=0, dbp=0, dwf=0, dbf=0, totalLoss = 0;
    for (int i = 0; i < 15; i++) {
      float predP = w_pump * train_x[i][0] + b_pump;
      float errP = predP - train_y[i][0];
      dwp += errP * train_x[i][0]; dbp += errP; totalLoss += errP * errP;
      float predF = w_fan * train_x[i][1] + b_fan;
      float errF = predF - train_y[i][1];
      dwf += errF * train_x[i][1]; dbf += errF; totalLoss += errF * errF;
    }
    w_pump -= lr * (dwp / 15); b_pump -= lr * (dbp / 15);
    w_fan -= lr * (dwf / 15);  b_fan -= lr * (dbf / 15);

    if (e % 20 == 0 || e == 99) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("AI Train: "); lcd.print(e + 1); lcd.print("%");
      lcd.setCursor(0, 1); lcd.print("Loss: "); lcd.print(totalLoss / 30.0, 4);
      delay(500); // Giảm delay để train nhanh hơn, bạn có thể chỉnh lại 1000 nếu muốn xem kỹ
    }
  }
  lcd.clear(); lcd.print("AI Trained OK!"); delay(1000);
}

/***************************************************
 * PHẦN BLYNK & ĐIỀU KHIỂN
 ***************************************************/
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V6, V10, V11, V12, V13, V14, V15);
  rtc.begin();
}

BLYNK_WRITE(V15) { digitalWrite(relayDen, param.asInt()); }

BLYNK_WRITE(V13) { 
  lastFanState = param.asInt(); 
  digitalWrite(relaySoLanh, lastFanState); 
  fanManualTimer = millis(); 
}

BLYNK_WRITE(V14) { 
  lastPumpState = param.asInt(); 
  digitalWrite(relayBom, lastPumpState); 
  pumpManualTimer = millis(); 
}

BLYNK_WRITE(V6)  { tankHeight = param.asInt(); }
BLYNK_WRITE(V2)  { myServo.write(param.asInt()); }
BLYNK_WRITE(V10) { updateSchedule(0, param); }
BLYNK_WRITE(V11) { updateSchedule(1, param); }
BLYNK_WRITE(V12) { updateSchedule(2, param); }

void updateSchedule(int index, BlynkParam param) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    schedHours[index] = t.getStartHour();
    schedMins[index] = t.getStartMinute();
    alreadyFired[index] = false;
  }
}

/***************************************************
 * XỬ LÝ HỆ THỐNG
 ***************************************************/
void processSystem() {
  // 1. XỬ LÝ NHIỆT ĐỘ
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    currentTemp = tempC;
    float pred_f = w_fan * (tempC / 60.0) + b_fan;
    
    if (millis() - fanManualTimer > overrideTime) {
      if (pred_f > 0.7 && !lastFanState) {
        lastFanState = true;
        digitalWrite(relaySoLanh, HIGH);
        Blynk.virtualWrite(V13, 1);
        // GỬI THÔNG BÁO VỀ ĐIỆN THOẠI
        Blynk.logEvent("thiet_bi_tu_dong", "AI: Nhiet do cao (" + String(tempC, 1) + "C). Da bat so lanh!");
      } 
      else if (pred_f < 0.1 && lastFanState) {
        lastFanState = false;
        digitalWrite(relaySoLanh, LOW);
        Blynk.virtualWrite(V13, 0);
      }
    }
    Blynk.virtualWrite(V5, tempC);
  }

  // 2. XỬ LÝ ĐỘ ĐỤC & TRẠNG THÁI NƯỚC (V8)
  int sensorValue = analogRead(TURBIDITY_PIN);
  float voltage = (sensorValue / 4095.0) * 3300.0;
  if (voltage >= 3100.0) currentNTU = 0;
  else if (voltage <= 1000.0) currentNTU = 3000;
  else currentNTU = (3100.0 - voltage) * (3000.0 / 2100.0);

  // Gửi trạng thái lên V8
  String waterStatus;
  if (currentNTU < 200) waterStatus = "SACH";
  else if (currentNTU <= 1000) waterStatus = "HOI BAN";
  else waterStatus = "RAT BAN";
  
  Blynk.virtualWrite(V8, waterStatus);
  Blynk.virtualWrite(V7, currentNTU);
  
  float pred_p = w_pump * (sensorValue / 4095.0) + b_pump;

  if (millis() - pumpManualTimer > overrideTime) {
    if (pred_p > 0.7 && !lastPumpState) {
      lastPumpState = true;
      digitalWrite(relayBom, HIGH);
      Blynk.virtualWrite(V14, 1);
      // GỬI THÔNG BÁO VỀ ĐIỆN THOẠI
      Blynk.logEvent("thiet_bi_tu_dong", "AI: Nuoc duc (" + String((int)currentNTU) + " NTU). Da bat may bom!");
    }
    else if (pred_p < 0.1 && lastPumpState) {
      lastPumpState = false;
      digitalWrite(relayBom, LOW);
      Blynk.virtualWrite(V14, 0);
    }
  }

  // 3. MỰC NƯỚC
  unsigned int distance = sonar.ping_cm();
  if (distance == 0) distance = MAX_DISTANCE;
  if (tankHeight > 0) waterLevel = tankHeight - distance;
  Blynk.virtualWrite(V4, waterLevel); 

  // 4. HIỂN THỊ LCD SAU KHI TRAIN
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(currentTemp, 1); lcd.print("C");
  lcd.setCursor(9, 0);
  lcd.print("N:"); lcd.print((int)currentNTU);
  
  lcd.setCursor(0, 1);
  lcd.print("P:"); lcd.print(lastPumpState ? "ON " : "OFF");
  lcd.setCursor(8, 1);
  lcd.print("F:"); lcd.print(lastFanState ? "ON " : "OFF");
}

void checkSchedule() {
  if (year() <= 1970) return;
  for (int i = 0; i < 3; i++) {
    if (hour() == schedHours[i] && minute() == schedMins[i]) {
      if (!alreadyFired[i]) {
        myServo.write(180); delay(2000); myServo.write(0);
        Blynk.logEvent("fish_fed_event", "Da cho ca an");
        alreadyFired[i] = true;
      }
    } else alreadyFired[i] = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(relayDen, OUTPUT); pinMode(relayBom, OUTPUT); pinMode(relaySoLanh, OUTPUT);
  digitalWrite(relayDen, LOW); digitalWrite(relayBom, LOW); digitalWrite(relaySoLanh, LOW);

  lcd.init(); lcd.backlight();
  sensors.begin();
  myServo.attach(servoPin, 500, 2400);
  randomSeed(analogRead(0));

  train_models();

  WiFi.begin(ssid, pass);
  Blynk.config(auth); 
  
  timer.setInterval(10000L, checkSchedule);
  timer.setInterval(2500L, processSystem);
}

void loop() {
  Blynk.run();
  timer.run();
}