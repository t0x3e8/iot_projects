/*
ESP32 BLE Client - PING Handshake Example
Connects to BLE Server and responds to "PING" with "PONG"
Maintains connection and waits for server messages every 30 seconds
*/

#include "BLEDevice.h"

// LED pin definition
const int ledPin = 0;  // GPIO pin for LED

// Server identification
static BLEUUID serviceUUID("f2e8d7c5-4b3a-9e1d-8f7c-6a5b4c3d2e1f");
static BLEUUID charUUID("a1b2c3d4-5e6f-7890-abcd-ef1234567890");

// Connection state variables
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

// Flag to track first PING after connection
static boolean firstPingReceived = false;

// Debug: Add timer to manually check characteristic value
static unsigned long lastDebugRead = 0;
static const unsigned long debugInterval = 5000;  // Check every 5 seconds

// LED blink function for PING indication
void blinkLED() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

// Client connection callbacks
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("✓ Connected to BLE Server");
    firstPingReceived = false;  // Reset flag on new connection
  }

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("✗ Disconnected from BLE Server");
  }
};

// Connect to the BLE Server
bool connectToServer() {
  Serial.print("🔗 Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  // Create BLE client
  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to server
  pClient->connect(myDevice);
  Serial.println("✓ Connected to server");
  pClient->setMTU(517);  // Set Maximum Transmission Unit

  // Get the service
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("❌ Failed to find service");
    pClient->disconnect();
    return false;
  }
  Serial.println("✓ Found service");

  // Get the characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("❌ Failed to find characteristic");
    pClient->disconnect();
    return false;
  }
  Serial.println("✓ Found characteristic");

  // Read the initial value of the characteristic
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("📋 Initial characteristic value: ");
    Serial.println(value);
    // Check if initial value is PING and respond
    if (value == "PING") {
      Serial.println("👋 Initial value is PING - sending PONG response");
      if (!firstPingReceived) {
        blinkLED();  // Blink LED only on first PING
        firstPingReceived = true;
      }
      if (pRemoteCharacteristic->canWrite()) {
        String response = "PONG";
        pRemoteCharacteristic->writeValue(response.c_str(), response.length());
        Serial.println("📤 Sent: PONG");
      }
    }
  }

  // Note: Using manual polling instead of notifications for ESP32C3 compatibility
  connected = true;
  return true;
}

// Scan for advertised devices
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("🔍 Found device: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Check if this device has our service
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.println("🎯 Found target server!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Starting BLE Client - PING Handshake Responder");

  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Initialize BLE
  BLEDevice::init("");

  // Setup BLE scanning
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349); // Set scan interval in milliseconds
  pBLEScan->setWindow(449); // Set scan window in milliseconds
  pBLEScan->setActiveScan(true);
  Serial.println("🔍 Starting scan for BLE servers...");
  pBLEScan->start(5, false); // Scan for 5 seconds, don't continue in background
}

void loop() {
  // Handle initial connection
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("🎉 Successfully connected! Waiting for PING messages every 30 seconds...");
      lastDebugRead = millis();  // Initialize debug timer
    } else {
      Serial.println("❌ Failed to connect to server");
    }
    doConnect = false;
  }

  // Debug: Manually read characteristic value every 5 seconds to test
  if (connected) {
    unsigned long currentTime = millis();
    if (currentTime - lastDebugRead >= debugInterval) {
      if (pRemoteCharacteristic->canRead()) {
        String currentValue = pRemoteCharacteristic->readValue();
        Serial.print("🔍 Debug read at ");
        Serial.print(currentTime / 1000);
        Serial.print("s: ");
        Serial.println(currentValue);
        // If we read PING, respond (this is our backup mechanism)
        if (currentValue == "PING") {
          Serial.println("👋 Found PING via manual read - sending response");
          if (!firstPingReceived) {
            blinkLED();  // Blink LED only on first PING
            firstPingReceived = true;
          }
          if (pRemoteCharacteristic->canWrite()) {
            String response = "PONG";
            pRemoteCharacteristic->writeValue(response.c_str(), response.length());
            Serial.println("📤 Sent: PONG");
          }
        }
      }
      lastDebugRead = currentTime;
    }
  }

  // Restart scanning if disconnected
  if (!connected && doScan) {
    Serial.println("🔄 Connection lost - restarting scan...");
    BLEDevice::getScan()->start(0);  // Scan indefinitely
  }

  delay(100);  // Small delay to prevent excessive CPU usage
}