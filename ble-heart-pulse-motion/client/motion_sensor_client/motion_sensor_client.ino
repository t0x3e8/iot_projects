#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Remote service and characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36/Users/jarek/Documents/IoT/Pulsing/Server_LED/Server_LED.inoe1-4688-b7f5-ea07361b26a8"

// PIR sensor pin
#define PIR_PIN 2  // Using GPIO2 for PIR sensor

// Timing settings
#define SCAN_TIME 5         // BLE scan time in seconds
#define COMMAND_DELAY 5000  // Delay between sending commands
#define PIR_INIT_TIME 30000 // PIR sensor initialization time (30 seconds)

// PIR state variables
bool pirState = LOW;         // Current PIR state
bool lastPirState = LOW;     // Previous PIR state
bool pirInitialized = false; // Flag to track if PIR has completed initialization
unsigned long pirInitStartTime = 0; // Time when PIR initialization started

// Global variables
BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteCharacteristic = NULL;
bool connected = false;
BLEAdvertisedDevice* myDevice = NULL;
bool doConnect = false;
bool sendStart = false;
bool sendStop = false;
unsigned long commandTime = 0;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      Serial.println("Found target device");
    }
  }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("Connected to server");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server");
  }
};

bool connectToServer() {
  Serial.println("Connecting to server...");
  
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  // Connect to the server
  if (!pClient->connect(myDevice)) {
    Serial.println("Connection failed");
    return false;
  }
  
  Serial.println("Connected to server");
  
  // Get service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("Service not found");
    pClient->disconnect();
    return false;
  }
  
  // Get characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Characteristic not found");
    pClient->disconnect();
    return false;
  }
  
  Serial.println("Connected successfully");
  return true;
}

// Simple write function
void writeCommand(const char* command) {
  if (connected && pRemoteCharacteristic != NULL) {
    Serial.print("Sending: ");
    Serial.println(command);
    pRemoteCharacteristic->writeValue(String(command));
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting BLE Client with PIR Sensor");
  
  // Initialize PIR sensor pin
  pinMode(PIR_PIN, INPUT);
  
  // Start PIR initialization timer
  pirInitStartTime = millis();
  Serial.println("PIR sensor initializing. Please wait 30 seconds...");
  
  // Initialize BLE
  BLEDevice::init("ESP32 BLE Client");
  
  // Setup scan
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  
  // Start scan
  pBLEScan->start(SCAN_TIME, false);
  Serial.println("Scanning...");
}

void loop() {
  // Handle PIR initialization period
  if (!pirInitialized) {
    if (millis() - pirInitStartTime > PIR_INIT_TIME) {
      pirInitialized = true;
      Serial.println("PIR sensor initialization complete");
    } else {
      // During initialization, just print a status message every 5 seconds
      static unsigned long lastStatusTime = 0;
      if (millis() - lastStatusTime > 5000) {
        lastStatusTime = millis();
        int remainingTime = (PIR_INIT_TIME - (millis() - pirInitStartTime)) / 1000;
        Serial.print("PIR initializing... ");
        Serial.print(remainingTime);
        Serial.println(" seconds remaining");
      }
      delay(100);
      return; // Skip the rest of the loop during initialization
    }
  }
  
  // Read PIR sensor state
  pirState = digitalRead(PIR_PIN);
  
  // Check if PIR state has changed
  if (pirState != lastPirState) {
    Serial.print("PIR state changed to: ");
    Serial.println(pirState ? "MOTION DETECTED" : "NO MOTION");
    
    // If motion detected, try to connect and send START
    if (pirState == HIGH) {
      if (!connected && myDevice != NULL) {
        doConnect = true;
      } else if (connected) {
        writeCommand("START");
        Serial.println("Motion detected - START command sent");
      }
    }
    // If no motion, send STOP if connected
    else if (connected) {
      writeCommand("STOP");
      Serial.println("No motion - STOP command sent");
    }
    
    lastPirState = pirState;
  }
  
  // Connect when device found and not connected
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to BLE server");
      // If motion is detected when we connect, send START right away
      if (pirState == HIGH) {
        writeCommand("START");
        Serial.println("Motion detected on connect - START command sent");
      }
    } else {
      Serial.println("Failed to connect");
    }
    doConnect = false;
  }
  
  // Reconnect if disconnected and we need to send a command
  if (!connected && myDevice != NULL && ((pirState == HIGH) || (pirState == LOW && lastPirState != pirState))) {
    Serial.println("Connection lost, attempting to reconnect...");
    if (connectToServer()) {
      if (pirState == HIGH) {
        writeCommand("START");
      } else {
        writeCommand("STOP");
      }
    }
  }
  
  // Heartbeat
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 10000) {
    lastHeartbeat = millis();
    Serial.println("Client running... PIR state: " + String(pirState ? "MOTION" : "NO MOTION"));
  }
  
  delay(100);
}




// // SIMPLE PIR SENSOR TEST

// // PIN definitions
// #define PIR_SENSOR_PIN 2  // Connect PIR sensor output to GPIO2/D0

// // Variables
// bool currentState = false;
// bool previousState = false;
// unsigned long lastPrintTime = 0;
// unsigned long startTime = 0;

// void setup() {
//   // Initialize serial communication
//   Serial.begin(115200);
//   delay(2000);  // Give serial time to initialize
  
//   // Initialize PIR sensor pin as input
//   pinMode(PIR_SENSOR_PIN, INPUT);
  
//   Serial.println("\n\nPIR Sensor Test Starting");
//   Serial.println("-------------------------");
//   Serial.println("Waiting for PIR to stabilize (30 seconds)...");
  
//   // Many PIR sensors need warm-up time
//   startTime = millis();
// }

// void loop() {
//   unsigned long currentTime = millis();
  
//   // During first 30 seconds, just show a countdown
//   if (currentTime - startTime < 30000) {
//     if (currentTime - lastPrintTime >= 5000) {  // Print every 5 seconds
//       lastPrintTime = currentTime;
//       int secondsLeft = 30 - ((currentTime - startTime) / 1000);
//       Serial.print("PIR warmup: ");
//       Serial.print(secondsLeft);
//       Serial.println(" seconds remaining");
//     }
//     return;  // Skip the rest during warmup
//   }
  
//   // Read the PIR sensor
//   currentState = digitalRead(PIR_SENSOR_PIN);
  
//   // Print current state every second
//   if (currentTime - lastPrintTime >= 1000) {
//     lastPrintTime = currentTime;
//     Serial.print("PIR Status: ");
//     Serial.print(currentState ? "Motion DETECTED" : "No Motion");
//     Serial.print(" (Raw value: ");
//     Serial.print(currentState);
//     Serial.println(")");
//   }
  
//   // If state changed, print immediately
//   if (currentState != previousState) {
//     Serial.print("CHANGE DETECTED! PIR Status now: ");
//     Serial.println(currentState ? "Motion DETECTED" : "No Motion");
//     previousState = currentState;
//   }
  
//   // Small delay for stability
//   delay(100);
// }


// BLE CLIENT TEST
// #include <BLEDevice.h>
// #include <BLEClient.h>
// #include <BLEScan.h>
// #include <BLEAdvertisedDevice.h>

// // Remote service and characteristic UUIDs
// #define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// // Timing settings
// #define SCAN_TIME 5         // BLE scan time in seconds
// #define COMMAND_DELAY 5000  // Delay between sending commands

// // Global variables
// BLEClient* pClient = NULL;
// BLERemoteCharacteristic* pRemoteCharacteristic = NULL;
// bool connected = false;
// BLEAdvertisedDevice* myDevice = NULL;
// bool doConnect = false;
// bool sendStart = false;
// bool sendStop = false;
// unsigned long commandTime = 0;

// class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
//   void onResult(BLEAdvertisedDevice advertisedDevice) {
//     Serial.print("Device found: ");
//     Serial.println(advertisedDevice.toString().c_str());

//     if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
//       BLEDevice::getScan()->stop();
//       myDevice = new BLEAdvertisedDevice(advertisedDevice);
//       doConnect = true;
//       Serial.println("Found target device");
//     }
//   }
// };

// class MyClientCallback : public BLEClientCallbacks {
//   void onConnect(BLEClient* pclient) {
//     connected = true;
//     Serial.println("Connected to server");
//   }

//   void onDisconnect(BLEClient* pclient) {
//     connected = false;
//     Serial.println("Disconnected from server");
//   }
// };

// bool connectToServer() {
//   Serial.println("Connecting to server...");
  
//   pClient = BLEDevice::createClient();
//   pClient->setClientCallbacks(new MyClientCallback());
  
//   // Connect to the server
//   if (!pClient->connect(myDevice)) {
//     Serial.println("Connection failed");
//     return false;
//   }
  
//   Serial.println("Connected to server");
  
//   // Get service
//   BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
//   if (pRemoteService == nullptr) {
//     Serial.println("Service not found");
//     pClient->disconnect();
//     return false;
//   }
  
//   // Get characteristic
//   pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
//   if (pRemoteCharacteristic == nullptr) {
//     Serial.println("Characteristic not found");
//     pClient->disconnect();
//     return false;
//   }
  
//   Serial.println("Connected successfully");
//   return true;
// }

// // Simple write function
// void writeCommand(const char* command) {
//   if (connected && pRemoteCharacteristic != NULL) {
//     Serial.print("Sending: ");
//     Serial.println(command);
//     pRemoteCharacteristic->writeValue(String(command));
//     delay(100);
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println("Starting BLE Client Test");
  
//   // Initialize BLE
//   BLEDevice::init("ESP32 BLE Client");
  
//   // Setup scan
//   BLEScan* pBLEScan = BLEDevice::getScan();
//   pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
//   pBLEScan->setActiveScan(true);
  
//   // Start scan
//   pBLEScan->start(SCAN_TIME, false);
//   Serial.println("Scanning...");
// }

// void loop() {
//   // Connect when device found
//   if (doConnect) {
//     if (connectToServer()) {
//       Serial.println("Connected to BLE server");
//       sendStart = true;
//       commandTime = millis();
//     } else {
//       Serial.println("Failed to connect");
//     }
//     doConnect = false;
//   }
  
//   // Handle sending START command
//   if (sendStart && millis() - commandTime > 2000) {
//     writeCommand("START");
//     sendStart = false;
//     sendStop = true;
//     commandTime = millis();
//   }
  
//   // Handle sending STOP command
//   if (sendStop && millis() - commandTime > COMMAND_DELAY) {
//     writeCommand("STOP");
//     sendStop = false;
//     commandTime = millis();
    
//     // Disconnect after sending STOP
//     if (connected) {
//       Serial.println("Disconnecting...");
//       pClient->disconnect();
//     }
    
//     // Restart scan after 2 seconds
//     delay(2000);
//     BLEDevice::getScan()->start(SCAN_TIME);
//   }
  
//   // Heartbeat
//   static unsigned long lastHeartbeat = 0;
//   if (millis() - lastHeartbeat > 5000) {
//     lastHeartbeat = millis();
//     Serial.println("Client running...");
//   }
  
//   delay(100);
// }