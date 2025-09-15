/* ESP32 BLE Server - PING Handshake Example
Sends "PING" every 30 seconds to connected client
Waits for "PONG" response
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID "f2e8d7c5-4b3a-9e1d-8f7c-6a5b4c3d2e1f"
#define CHARACTERISTIC_UUID "a1b2c3d4-5e6f-7890-abcd-ef1234567890"

// Global variables
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// PIR motion sensor
const int pirPin = 2;
bool motionDetected = false;
bool lastMotionState = false;
unsigned long lastMotionCheck = 0;
const unsigned long motionCheckInterval = 100; // Check motion every 100ms

// Handshake protocol - PING first, then motion detection
bool handshakeCompleted = false;
bool pingSent = false;
unsigned long connectionTime = 0;
const unsigned long handshakeDelay = 3000; // 3 seconds after connection

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
      
      // Check for PONG response during handshake
      if (receivedMessage == "PONG" && !handshakeCompleted) {
        Serial.println("🎉 Handshake completed! Switching to motion detection...");
        handshakeCompleted = true;
        // Initialize motion state
        lastMotionState = digitalRead(pirPin);
      }
    }
  }
};



void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Starting BLE Server - Motion Detection");

  // Initialize PIR sensor
  pinMode(pirPin, INPUT);
  Serial.println("📡 PIR Motion Sensor initialized on pin 2");

  // Initialize BLE
  BLEDevice::init("ESP32-BLE-Server");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic with read and write properties only
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE
  );

  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
  pCharacteristic->setValue("BLE Server Ready");

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true); // Enable scan response data for more device info
  pAdvertising->setMinPreferred(0x06); // Set minimum connection interval preference
  pAdvertising->setMinPreferred(0x12); // Set maximum connection interval preference
  BLEDevice::startAdvertising();

  Serial.println("📡 BLE Server advertising - waiting for client...");
}

void loop() {
  unsigned long currentTime = millis();

  if (deviceConnected) {
    // Phase 1: Handshake (PING-PONG)
    if (!handshakeCompleted) {
      // Send PING 3 seconds after connection
      if (!pingSent && (currentTime - connectionTime >= handshakeDelay)) {
        Serial.println("👋 Sending PING for handshake");
        pCharacteristic->setValue("PING");
        pingSent = true;
      }
    }
    // Phase 2: Motion detection (after handshake)
    else {
      // Check motion sensor
      if (currentTime - lastMotionCheck >= motionCheckInterval) {
        motionDetected = digitalRead(pirPin);
        
        // Debug: Print sensor reading every 5 seconds
        static unsigned long lastDebugTime = 0;
        if (currentTime - lastDebugTime >= 5000) {
          Serial.print("🔍 PIR sensor reading: ");
          Serial.print(motionDetected);
          Serial.print(" (pin ");
          Serial.print(pirPin);
          Serial.println(")");
          lastDebugTime = currentTime;
        }
        
        // Motion state changed
        if (motionDetected != lastMotionState) {
          if (motionDetected) {
            Serial.println("🚶 Motion detected - sending START");
            pCharacteristic->setValue("START");
          } else {
            Serial.println("🛑 Motion stopped - sending STOP");
            pCharacteristic->setValue("STOP");
          }
          lastMotionState = motionDetected;
        }
        lastMotionCheck = currentTime;
      }
    }
  }

  // Handle reconnection advertising
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);  // Give bluetooth stack time to reset
    BLEDevice::startAdvertising();
    Serial.println("🔄 Restarting advertising...");
    oldDeviceConnected = deviceConnected;
  }

  // Update connection state
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(100);  // Small delay to prevent excessive CPU usage
}