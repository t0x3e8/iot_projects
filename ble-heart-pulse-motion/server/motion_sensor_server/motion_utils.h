#ifndef MOTION_UTILS_H
#define MOTION_UTILS_H

#include <Arduino.h>

// Motion sensor utilities
void initMotionSensor(int pin);
bool checkMotion(int pin);
void printMotionDebug(int pin, bool currentState);

#endif