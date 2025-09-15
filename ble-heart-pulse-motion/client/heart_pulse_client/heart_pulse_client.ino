/*
ESP32 BLE Client - Heart Pulse Controller
Simple structure with extracted LED effects
*/

#include "BLEDevice.h"
#include "led_effects.h"

// LED pin definition
const int ledPin = 0;

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

// Timer to read server every 3 seconds
static unsigned long lastServerRead = 0;
static const unsigned long serverReadInterval = 3000;

// Forward declaration
void handleServerMessage(String message, bool isInitialValue = false);

// Client connection callbacks
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("✓ Connected to BLE Server");
    firstPingReceived = false;
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

  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println("✓ Connected to server");
  pClient->setMTU(517);

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("❌ Failed to find service");
    pClient->disconnect();
    return false;
  }
  Serial.println("✓ Found service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("❌ Failed to find characteristic");
    pClient->disconnect();
    return false;
  }
  Serial.println("✓ Found characteristic");

  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("📋 Initial characteristic value: ");
    Serial.println(value);
    
    handleServerMessage(value, true);
  }

  connected = true;
  return true;
}

// Handle messages from server
void handleServerMessage(String message, bool isInitialValue) {
  if (message == "START") {
    Serial.println("🚶 Motion detected - starting heartbeat effect");
    startHeartbeat(ledPin);
  } else if (message == "STOP") {
    Serial.println("🛑 Motion stopped - stopping heartbeat effect");
    stopHeartbeat(ledPin);
  } else if (message == "PING") {
    Serial.println("👋 Received PING - sending PONG response");
    if (!firstPingReceived) {
      Serial.println("🔥 First PING detected - blinking LED!");
      blinkLED(ledPin);
      firstPingReceived = true;
    } else {
      Serial.println("📢 Subsequent PING - no blink");
    }
    if (pRemoteCharacteristic->canWrite()) {
      String response = "PONG";
      pRemoteCharacteristic->writeValue(response.c_str(), response.length());
      Serial.println("📤 Sent: PONG");
    }
  }
}

// Scan for advertised devices
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("🔍 Found device: ");
    Serial.println(advertisedDevice.toString().c_str());

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
  Serial.println("🚀 Starting BLE Client - Heart Pulse Controller");

  initLED(ledPin);

  BLEDevice::init("");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  Serial.println("🔍 Starting scan for BLE servers...");
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("🎉 Successfully connected! Reading server every 3 seconds for motion updates...");
      lastServerRead = millis();
    } else {
      Serial.println("❌ Failed to connect to server");
    }
    doConnect = false;
  }

  if (connected) {
    unsigned long currentTime = millis();
    if (currentTime - lastServerRead >= serverReadInterval) {
      if (pRemoteCharacteristic->canRead()) {
        String currentValue = pRemoteCharacteristic->readValue();
        Serial.print("📖 Reading server at ");
        Serial.print(currentTime / 1000);
        Serial.print("s: ");
        Serial.println(currentValue);
        
        handleServerMessage(currentValue, false);
      }
      lastServerRead = currentTime;
    }
  }

  updateHeartbeat(ledPin);

  if (!connected && doScan) {
    Serial.println("🔄 Connection lost - restarting scan...");
    BLEDevice::getScan()->start(0);
  }

  delay(1);
}