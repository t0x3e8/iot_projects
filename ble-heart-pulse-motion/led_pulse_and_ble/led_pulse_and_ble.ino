// Heart pulse effect with BLE Control
// Use a BLE app to start/stop the pulsing

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// LED settings
const int ledPin = 0;  // GPIO pin for LED

// Variables for the heart pulse effect
bool isPulsing = false;
unsigned long previousMillis = 0;
int pulseState = 0;
int brightness = 0;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// BLE Server callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

// BLE Characteristic callbacks
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      // Get value as a C-style string
      uint8_t* data = pCharacteristic->getData();
      size_t len = pCharacteristic->getLength();
      
      if (len > 0) {
        Serial.print("Received Value: ");
        // Create a temporary buffer with space for null terminator
        char rxValue[len + 1];
        
        // Copy the data and add null terminator
        memcpy(rxValue, data, len);
        rxValue[len] = '\0';
        
        Serial.println(rxValue);

        // Check command using standard string comparison
        if (strstr(rxValue, "START") != NULL) {
          Serial.println("Starting heart pulse");
          isPulsing = true;
        } else if (strstr(rxValue, "STOP") != NULL) {
          Serial.println("Stopping heart pulse");
          isPulsing = false;
          analogWrite(ledPin, 0);  // Turn off LED
        }
      }
    }
};

void setup() {
  // Initialize serial
  Serial.begin(115200);

  Serial.println("START---------------");
  
  // Configure LED pin as output
  pinMode(ledPin, OUTPUT);
  
  // Create the BLE Device
  BLEDevice::init("ESP32 Heart Pulse");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  
  // Set callbacks
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Set initial value (using raw bytes)
  uint8_t initialValue[] = "Ready";
  pCharacteristic->setValue(initialValue, sizeof(initialValue)-1);
  
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Heart Pulse service started");
  Serial.println("Use a BLE app to connect and send 'START' or 'STOP'");
}

void loop() {
  // Disconnection handling
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the Bluetooth stack time to get ready
    pServer->startAdvertising(); // Restart advertising
    Serial.println("Restarting advertising");
    oldDeviceConnected = deviceConnected;
  }
  
  // Connection handling
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  // Heart pulse effect
  if (isPulsing) {
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
    delay(1);  // Small delay for smooth transitions
  }
}