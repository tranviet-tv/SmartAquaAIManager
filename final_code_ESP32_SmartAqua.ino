/*************************************************************
  SMART AQUACULTURE IOT SYSTEM
  Description: ESP32 Firmware for monitoring water quality, 
  controlling relays (pump, fan, light), and feeding schedule.
  Integrates with Blynk and Firebase RTDB.
 *************************************************************/

// --- 1. CREDENTIALS CONFIGURATION (DO NOT SHARE PUBLICLY) ---
// Note: Replace these placeholders with your actual credentials
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// Do not include 'https://' here
#define FIREBASE_HOST "YOUR_PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "YOUR_FIREBASE_DATABASE_SECRET"

// -----------------------------------------------------------

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
#include <FirebaseESP32.h>

// --- Pin Definitions ---
#define TRIGGER_PIN 5
#define ECHO_PIN 18
#define MAX_DISTANCE 200
#define ONE_WIRE_BUS 14
#define TURBIDITY_PIN 34

const int relayFan   = 25;  // V13
const int relayPump  = 26;  // V14
const int relayLight = 27;  // V15
const int servoPin   = 19;

// --- Object Initializations ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo myServo;
WidgetRTC rtc;
BlynkTimer timer;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

FirebaseData fbdo;
FirebaseAuth fbauth;
FirebaseConfig fbconfig;

// --- System Variables ---
int tankHeight   = 0;
int waterLevel   = 0;
float currentTemp = 0.0;
float currentNTU  = 0.0;
int schedHours[3] = { -1, -1, -1 }, schedMins[3] = { -1, -1, -1 };
bool alreadyFired[3] = { false, false, false };

bool lastPumpState = false;
bool lastFanState  = false;

unsigned long pumpManualTimer = 0;
unsigned long fanManualTimer  = 0;
const long overrideTime = 30000; // 30 seconds manual override

// AI Weights fetched from Firebase
float w_pump, b_pump, w_fan, b_fan;

// --- Function Prototypes ---
void checkSchedule();
void processSystem();
void updateSchedule(int index, BlynkParam param);
void updateAIFromFirebase();

/***************************************************
 * FIREBASE: FETCH AI WEIGHTS
 ***************************************************/
void updateAIFromFirebase() {
  if (Firebase.ready()) {
    Serial.println(">>> Fetching AI weights from Firebase...");

    // Fetch w_fan
    if (Firebase.getFloat(fbdo, "/AI_Model/w_fan")) { w_fan = fbdo.floatData(); }
    else { Serial.print("w_fan Error: "); Serial.println(fbdo.errorReason()); }

    // Fetch remaining weights
    if (Firebase.getFloat(fbdo, "/AI_Model/b_fan")) b_fan = fbdo.floatData();
    if (Firebase.getFloat(fbdo, "/AI_Model/w_pump")) w_pump = fbdo.floatData();
    if (Firebase.getFloat(fbdo, "/AI_Model/b_pump")) b_pump = fbdo.floatData();

    Serial.println("--- WEIGHTS UPDATED ---");
    Serial.printf("Fan: w=%.4f, b=%.4f | Pump: w=%.4f, b=%.4f\n", w_fan, b_fan, w_pump, b_pump);
  }
}

/***************************************************
 * BLYNK: CLOUD SYNC & CONTROL
 ***************************************************/
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V6, V10, V11, V12, V13, V14, V15);
  rtc.begin(); // Sync the real time clock
}

BLYNK_WRITE(V15) { digitalWrite(relayLight, param.asInt()); }

BLYNK_WRITE(V13) { 
  lastFanState = param.asInt(); 
  digitalWrite(relayFan, lastFanState); 
  fanManualTimer = millis(); 
}

BLYNK_WRITE(V14) { 
  lastPumpState = param.asInt(); 
  digitalWrite(relayPump, lastPumpState); 
  pumpManualTimer = millis(); 
}

BLYNK_WRITE(V6) { tankHeight = param.asInt(); }
BLYNK_WRITE(V2) { myServo.write(param.asInt()); }

// Scheduling pins
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
 * MAIN LOGIC: SENSORS & AI ACTUATION
 ***************************************************/
void processSystem() {
  // --- 1. TEMPERATURE & FAN CONTROL ---
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC != DEVICE_DISCONNECTED_C) {
    currentTemp = tempC;

    // Normalize temperature (20-40 range) to match Python model
    float temp_norm = (tempC - 20.0) / (40.0 - 20.0);

    // AI Prediction: Linear Regression formula (y = w*x + b)
    float pred_f = (w_fan * temp_norm) + b_fan;

    // Automatically control Fan based on AI Prediction (if manual override expired)
    if (millis() - fanManualTimer > overrideTime) {
      if (pred_f > 0.8 && !lastFanState) {
        lastFanState = true;
        digitalWrite(relayFan, HIGH);
        Blynk.virtualWrite(V13, 1);
        Blynk.logEvent("thiet_bi_tu_dong", "AI: High Temp (" + String(tempC, 1) + "C). Fan turned ON!");
      } else if (pred_f < 0.2 && lastFanState) {
        lastFanState = false;
        digitalWrite(relayFan, LOW);
        Blynk.virtualWrite(V13, 0);
      }
    }
    Blynk.virtualWrite(V5, tempC);
  }

  // --- 2. TURBIDITY & PUMP CONTROL ---
  int adc_raw = analogRead(TURBIDITY_PIN);

  // Calculate NTU for Display
  float voltage = (adc_raw / 4095.0) * 3300.0;
  if (voltage >= 3100.0) currentNTU = 0;
  else if (voltage <= 1000.0) currentNTU = 3000;
  else currentNTU = (3100.0 - voltage) * (3000.0 / 2100.0);

  // Process AI for Pump (SEN0189 trait: murkier water = lower ADC)
  int reversed_adc = 4095 - adc_raw;
  float turb_norm = (float)reversed_adc / 4095.0;
  
  // Predict using Pump weights
  float pred_p = (w_pump * turb_norm) + b_pump;

  // Classify Water Status for Dashboard Display
  String waterStatus;
  if (pred_p < 0.3) {
    waterStatus = "CLEAN";
  } else if (pred_p <= 0.7) {
    waterStatus = "DIRTY";
  } else {
    waterStatus = "VERY DIRTY";
  }

  Blynk.virtualWrite(V8, waterStatus);
  Blynk.virtualWrite(V7, currentNTU);

  // Automatically control Pump (Filter)
  if (millis() - pumpManualTimer > overrideTime) {
    if (pred_p > 0.7 && !lastPumpState) {
      lastPumpState = true;
      digitalWrite(relayPump, HIGH);
      Blynk.virtualWrite(V14, 1);
      Blynk.logEvent("thiet_bi_tu_dong", "AI: Dirty Water (" + String((int)currentNTU) + " NTU). Filter ON!");
    } else if (pred_p < 0.2 && lastPumpState) {
      lastPumpState = false;
      digitalWrite(relayPump, LOW);
      Blynk.virtualWrite(V14, 0);
    }
  }

  // --- 3. WATER LEVEL (SONAR) ---
  unsigned int distance = sonar.ping_cm();
  if (distance == 0) distance = MAX_DISTANCE;
  if (tankHeight > 0) waterLevel = tankHeight - distance;
  
  Blynk.virtualWrite(V4, waterLevel);

  // --- 4. LCD UPDATE ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(currentTemp, 1);
  lcd.print("C");
  lcd.setCursor(9, 0);
  lcd.print("N:");
  lcd.print((int)currentNTU);

  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(lastPumpState ? "ON " : "OFF");
  lcd.setCursor(8, 1);
  lcd.print("F:");
  lcd.print(lastFanState ? "ON " : "OFF");
}

/***************************************************
 * FEEDING SCHEDULE
 ***************************************************/
void checkSchedule() {
  if (year() <= 1970) return; // Ignore if RTC is not synced
  for (int i = 0; i < 3; i++) {
    if (hour() == schedHours[i] && minute() == schedMins[i]) {
      if (!alreadyFired[i]) {
        myServo.write(180);
        delay(2000);
        myServo.write(0);
        Blynk.logEvent("fish_fed_event", "Automated Feeding Triggered");
        alreadyFired[i] = true;
      }
    } else {
      alreadyFired[i] = false;
    }
  }
}

/***************************************************
 * SETUP & MAIN LOOP
 ***************************************************/
void setup() {
  Serial.begin(115200);
  
  // Initialize Relays
  pinMode(relayLight, OUTPUT);
  pinMode(relayPump, OUTPUT);
  pinMode(relayFan, OUTPUT);
  digitalWrite(relayLight, LOW);
  digitalWrite(relayPump, LOW);
  digitalWrite(relayFan, LOW);

  // Initialize Modules
  lcd.init();
  lcd.backlight();
  sensors.begin();
  myServo.attach(servoPin, 500, 2400);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  WiFi.setSleep(false);

  // Connect to Blynk
  Blynk.config(BLYNK_AUTH_TOKEN);

  // Connect to Firebase
  fbconfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  fbconfig.database_url = "https://" FIREBASE_HOST;
  Firebase.begin(&fbconfig, &fbauth);
  Firebase.setDoubleDigits(5);
  Firebase.reconnectWiFi(true);

  // Increase payload size capability
  fbdo.setResponseSize(1024);
  fbdo.setBSSLBufferSize(256, 256);

  // Fetch initial weights immediately on startup
  updateAIFromFirebase();

  // Configure Timers
  timer.setInterval(10000L, checkSchedule);       // RTC Feeding check
  timer.setInterval(2500L, processSystem);        // Sensor & AI logic loop
  timer.setInterval(60000L, updateAIFromFirebase); // Refresh weights every 60s
}

void loop() {
  Blynk.run();
  timer.run();
}