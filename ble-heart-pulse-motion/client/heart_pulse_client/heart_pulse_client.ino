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

// Timer to read server every 3 seconds
static unsigned long lastServerRead = 0;
static const unsigned long serverReadInterval = 3000;  // Check every 3 seconds

// Heartbeat effect variables
bool heartbeatActive = false;
int brightness = 0;
int pulseState = 0;
unsigned long previousMillis = 0;

// LED control functions
void setLEDAnalog(int brightness) {
  analogWrite(ledPin, brightness);
}

void turnOffLED() {
  analogWrite(ledPin, 0);
}

void startHeartbeat() {
  heartbeatActive = true;
  brightness = 0;
  pulseState = 0;
  previousMillis = millis();
  Serial.println("💓 Starting heartbeat effect");
}

void stopHeartbeat() {
  heartbeatActive = false;
  brightness = 0;
  pulseState = 0;
  analogWrite(ledPin, 0);
  Serial.println("💔 Stopping heartbeat effect");
}

void updateHeartbeat() {
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
  
  analogWrite(ledPin, brightness);
}

// LED blink function for PING indication
void blinkLED() {
  for (int i = 0; i < 3; i++) {
    analogWrite(ledPin, 255);  // Full brightness
    delay(100);
    analogWrite(ledPin, 0);    // Off
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
    
    // Handle motion detection messages
    if (value == "START") {
      Serial.println("🚶 Motion detected - starting heartbeat effect");
      startHeartbeat();
    } else if (value == "STOP") {
      Serial.println("🛑 Motion stopped - stopping heartbeat effect");
      stopHeartbeat();
    }
    // Handle PING messages (for testing)
    else if (value == "PING") {
      Serial.println("👋 Initial value is PING - sending PONG response");
      if (!firstPingReceived) {
        Serial.println("🔥 First PING detected - blinking LED!");
        blinkLED();  // Blink LED only on first PING
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
      Serial.println("🎉 Successfully connected! Reading server every 3 seconds for motion updates...");
      lastServerRead = millis();  // Initialize server read timer
    } else {
      Serial.println("❌ Failed to connect to server");
    }
    doConnect = false;
  }

  // Read server every 3 seconds for motion updates
  if (connected) {
    unsigned long currentTime = millis();
    if (currentTime - lastServerRead >= serverReadInterval) {
      if (pRemoteCharacteristic->canRead()) {
        String currentValue = pRemoteCharacteristic->readValue();
        Serial.print("📖 Reading server at ");
        Serial.print(currentTime / 1000);
        Serial.print("s: ");
        Serial.println(currentValue);
        
        // Handle motion detection messages
        if (currentValue == "START") {
          Serial.println("🚶 Motion detected - starting heartbeat effect");
          startHeartbeat();
        } else if (currentValue == "STOP") {
          Serial.println("🛑 Motion stopped - stopping heartbeat effect");
          stopHeartbeat();
        }
        // Handle PING messages (for testing)
        else if (currentValue == "PING") {
          Serial.println("👋 Found PING via manual read - sending response");
          if (!firstPingReceived) {
            Serial.println("🔥 First PING detected via polling - blinking LED!");
            blinkLED();  // Blink LED only on first PING
            firstPingReceived = true;
          } else {
            Serial.println("📢 Subsequent PING via polling - no blink");
          }
          if (pRemoteCharacteristic->canWrite()) {
            String response = "PONG";
            pRemoteCharacteristic->writeValue(response.c_str(), response.length());
            Serial.println("📤 Sent: PONG");
          }
        }
      }
      lastServerRead = currentTime;
    }
  }

  // Update heartbeat effect continuously
  updateHeartbeat();

  // Restart scanning if disconnected
  if (!connected && doScan) {
    Serial.println("🔄 Connection lost - restarting scan...");
    BLEDevice::getScan()->start(0);  // Scan indefinitely
  }

  delay(1);  // Small delay for smooth heartbeat transitions
}