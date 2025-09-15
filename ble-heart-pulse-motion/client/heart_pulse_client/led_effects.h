#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include <Arduino.h>

// LED Effects - Reusable component
void initLED(int pin);
void startHeartbeat(int pin);
void stopHeartbeat(int pin);
void updateHeartbeat(int pin);
void blinkLED(int pin);
bool isHeartbeatActive();

#endif