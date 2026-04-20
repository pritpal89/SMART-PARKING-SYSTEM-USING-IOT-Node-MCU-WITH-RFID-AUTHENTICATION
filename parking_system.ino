#define BLYNK_TEMPLATE_ID "TMPL3ffnYkaqt"
#define BLYNK_TEMPLATE_NAME "Smart Parking"
#define BLYNK_AUTH_TOKEN "FiFi_WwJfH_gtoolB-RwPz0KXKS1OZ0A"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Harish";
char pass[] = "987654321";

// Pins
#define IR_ENTRY   D5
#define IR_EXIT    D6
#define SERVO_PIN  D2

// Parking
const int TOTAL_SLOTS = 8;
int availableSlots = TOTAL_SLOTS;

// Servo angles
const int GATE_OPEN_ANGLE  = 0;
const int GATE_CLOSE_ANGLE = 90;

// Flags
bool entryTriggered = false;
bool exitTriggered  = false;
bool gateOpen = false;

// Timing
unsigned long gateOpenedAt = 0;
const unsigned long gateOpenDuration = 2000;

// Sensor cooldown (anti-multiple trigger)
unsigned long lastEntryTime = 0;
unsigned long lastExitTime = 0;
const unsigned long sensorCooldown = 1500; // 1.5 sec

Servo gateServo;
BlynkTimer timer;

// ---------------- FUNCTIONS ----------------

void updateBlynk(String statusText) {
  Blynk.virtualWrite(V0, availableSlots);
  Blynk.virtualWrite(V1, TOTAL_SLOTS - availableSlots);
  Blynk.virtualWrite(V2, statusText);
}

void openGate() {
  gateServo.write(GATE_OPEN_ANGLE);
  gateOpen = true;
  gateOpenedAt = millis();
}

void closeGate() {
  gateServo.write(GATE_CLOSE_ANGLE);
  gateOpen = false;
  updateBlynk("Gate Closed");
}

void handleEntry() {
  if (availableSlots > 0) {
    availableSlots--;
    openGate();
    updateBlynk("Car Entered");
    Blynk.logEvent("car_entry", "A car entered the parking area.");
  } else {
    updateBlynk("Parking Full");
    Blynk.logEvent("parking_full", "Parking is full. Entry denied.");
  }
}

void handleExit() {
  if (availableSlots < TOTAL_SLOTS) {
    availableSlots++;
  }

  openGate();
  updateBlynk("Car Exited");
  Blynk.logEvent("car_exit", "A car exited the parking area.");
}

// ----------- SENSOR CHECK (STABLE VERSION) -----------

void checkSensors() {
  bool entryState = (digitalRead(IR_ENTRY) == LOW);
  bool exitState  = (digitalRead(IR_EXIT) == LOW);

  unsigned long currentMillis = millis();

  // ENTRY
  if (entryState && !entryTriggered && !gateOpen &&
      (currentMillis - lastEntryTime > sensorCooldown)) {

    entryTriggered = true;
    lastEntryTime = currentMillis;
    handleEntry();
  }

  if (!entryState) {
    entryTriggered = false;
  }

  // EXIT
  if (exitState && !exitTriggered && !gateOpen &&
      (currentMillis - lastExitTime > sensorCooldown)) {

    exitTriggered = true;
    lastExitTime = currentMillis;
    handleExit();
  }

  if (!exitState) {
    exitTriggered = false;
  }
}

// ----------- GATE AUTO CLOSE -----------

void manageGate() {
  if (gateOpen && (millis() - gateOpenedAt >= gateOpenDuration)) {
    closeGate();
  }
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(115200);

  pinMode(IR_ENTRY, INPUT);
  pinMode(IR_EXIT, INPUT);

  gateServo.attach(SERVO_PIN);
  closeGate();

  Blynk.begin(auth, ssid, pass);

  timer.setInterval(300L, checkSensors);
  timer.setInterval(100L, manageGate);

  updateBlynk("System Ready");
}

// ---------------- LOOP ----------------

void loop() {
  Blynk.run();
  timer.run();
}