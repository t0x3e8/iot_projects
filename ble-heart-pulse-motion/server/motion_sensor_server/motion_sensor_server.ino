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

// Timing for PING messages - first after 3 seconds, then every 30 seconds
unsigned long lastPingTime = 0;
const unsigned long firstPingDelay = 3000;   // 3 seconds for first PING
const unsigned long pingInterval = 30000;    // 30 seconds for subsequent PINGs
bool firstPingSent = false;

// Server connection callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("✓ Client connected - first PING will be sent in 3 seconds");
    lastPingTime = millis();  // Reset timer on new connection
    firstPingSent = false;    // Reset first ping flag
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

      // Check for expected PONG response
      if (receivedMessage == "PONG") {
        Serial.println("🎉 Handshake completed successfully!");
      }
    }
  }
};



void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Starting BLE Server - PING Handshake every 30 seconds");

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

  // Send PING: first after 3 seconds, then every 30 seconds
  if (deviceConnected) {
    unsigned long targetDelay = firstPingSent ? pingInterval : firstPingDelay;
    if (currentTime - lastPingTime >= targetDelay) {
      Serial.println("👋 Sending PING to client");
      pCharacteristic->setValue("PING");
      lastPingTime = currentTime;
      firstPingSent = true;
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