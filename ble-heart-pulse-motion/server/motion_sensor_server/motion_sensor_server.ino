/* ESP32 BLE Server - Motion Detection
Simple structure with extracted motion utilities
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "motion_utils.h"

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID "f2e8d7c5-4b3a-9e1d-8f7c-6a5b4c3d2e1f"
#define CHARACTERISTIC_UUID "a1b2c3d4-5e6f-7890-abcd-ef1234567890"

// Global variables
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// PIR motion sensor
const int pirPin = 2;
bool lastMotionState = false;
unsigned long lastMotionCheck = 0;
const unsigned long motionCheckInterval = 100;

// Handshake protocol
bool handshakeCompleted = false;
bool pingSent = false;
unsigned long connectionTime = 0;
const unsigned long handshakeDelay = 3000;



// Server connection callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    handshakeCompleted = false;
    pingSent = false;
    connectionTime = millis();
    Serial.println("✓ Client connected - starting handshake protocol");
    pCharacteristic->setValue("BLE Server Ready");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("✗ Client disconnected");
  }
};

// Characteristic read/write callbacks
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String receivedMessage = pCharacteristic->getValue();

    if (receivedMessage.length() > 0) {
      Serial.print("📨 Received: ");
      Serial.println(receivedMessage);
      
      if (receivedMessage == "PONG" && !handshakeCompleted) {
        Serial.println("🎉 Handshake completed! Switching to motion detection...");
        handshakeCompleted = true;
        lastMotionState = checkMotion(pirPin);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Starting BLE Server - Motion Detection");

  initMotionSensor(pirPin);

  // Initialize BLE
  BLEDevice::init("ESP32-BLE-Server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->setValue("BLE Server Ready");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("📡 BLE Server advertising - waiting for client...");
}

void loop() {
  unsigned long currentTime = millis();

  if (deviceConnected) {
    if (!handshakeCompleted) {
      if (!pingSent && (currentTime - connectionTime >= handshakeDelay)) {
        Serial.println("👋 Sending PING for handshake");
        pCharacteristic->setValue("PING");
        pingSent = true;
      }
    } else {
      if (currentTime - lastMotionCheck >= motionCheckInterval) {
        bool currentMotion = checkMotion(pirPin);
        
        printMotionDebug(pirPin, currentMotion);
        
        // Continuously broadcast current motion state
        if (currentMotion) {
          pCharacteristic->setValue("START");
          if (currentMotion != lastMotionState) {
            Serial.println("🚶 Motion detected - continuously sending START");
          }
        } else {
          pCharacteristic->setValue("STOP");
          if (currentMotion != lastMotionState) {
            Serial.println("🛑 Motion stopped - continuously sending STOP");
          }
        }
        lastMotionState = currentMotion;
        lastMotionCheck = currentTime;
      }
    }
  }

  // Handle reconnection advertising
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    BLEDevice::startAdvertising();
    Serial.println("🔄 Restarting advertising...");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(100);
}