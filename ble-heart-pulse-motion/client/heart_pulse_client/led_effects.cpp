#include "led_effects.h"

// Static variables for LED effects
static bool heartbeatActive = false;
static int brightness = 0;
static int pulseState = 0;
static unsigned long previousMillis = 0;

void initLED(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  Serial.print("💡 LED initialized on pin ");
  Serial.println(pin);
}

void startHeartbeat(int pin) {
  heartbeatActive = true;
  brightness = 0;
  pulseState = 0;
  previousMillis = millis();
  Serial.println("💓 Starting heartbeat effect");
}

void stopHeartbeat(int pin) {
  heartbeatActive = false;
  brightness = 0;
  pulseState = 0;
  analogWrite(pin, 0);
  Serial.println("💔 Stopping heartbeat effect");
}

void updateHeartbeat(int pin) {
  if (!heartbeatActive) return;
  
  unsigned long currentMillis = millis();
  
  switch (pulseState) {
    case 0:  // First beat rise
      brightness += 15;
      if (brightness >= 255) {
        brightness = 255;
        pulseState = 1;
      }
      break;
      
    case 1:  // First beat fall
      brightness -= 8;
      if (brightness <= 0) {
        brightness = 0;
        pulseState = 2;
        previousMillis = currentMillis;
      }
      break;
      
    case 2:  // Pause between beats
      if (currentMillis - previousMillis >= 120) {
        pulseState = 3;
      }
      break;
      
    case 3:  // Second beat rise
      brightness += 15;
      if (brightness >= 180) {
        brightness = 180;
        pulseState = 4;
      }
      break;
      
    case 4:  // Second beat fall
      brightness -= 8;
      if (brightness <= 0) {
        brightness = 0;
        pulseState = 5;
        previousMillis = currentMillis;
      }
      break;
      
    case 5:  // Long pause before next heartbeat
      if (currentMillis - previousMillis >= 800) {
        pulseState = 0;
      }
      break;
  }
  
  analogWrite(pin, brightness);
}

void blinkLED(int pin) {
  for (int i = 0; i < 3; i++) {
    analogWrite(pin, 255);
    delay(100);
    analogWrite(pin, 0);
    delay(100);
  }
}

bool isHeartbeatActive() {
  return heartbeatActive;
}