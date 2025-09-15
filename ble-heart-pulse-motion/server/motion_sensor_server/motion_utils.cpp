#include "motion_utils.h"

void initMotionSensor(int pin) {
  pinMode(pin, INPUT);
  Serial.print("📡 PIR Motion Sensor initialized on pin ");
  Serial.println(pin);
}

bool checkMotion(int pin) {
  return digitalRead(pin);
}

void printMotionDebug(int pin, bool currentState) {
  static unsigned long lastDebugTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastDebugTime >= 5000) {
    Serial.print("🔍 PIR sensor reading: ");
    Serial.print(currentState);
    Serial.print(" (pin ");
    Serial.print(pin);
    Serial.println(")");
    lastDebugTime = currentTime;
  }
}