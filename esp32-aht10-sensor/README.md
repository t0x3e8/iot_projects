# ESP32 AHT10 Temperature/Humidity Sensor

An IoT sensor project that reads temperature and humidity data from an AHT10 sensor using ESP32-C3 and posts the data to a remote server via WiFi.

## Features

- **AHT10 Sensor Integration**: Accurate temperature and humidity readings
- **WiFi Management**: Automatic WiFi connection with captive portal setup
- **Remote Data Posting**: Sends sensor data to API endpoint every 30 seconds
- **Reset Button**: 15-second hold to reset WiFi settings
- **Error Handling**: Robust error handling for sensor and network failures
- **Power Efficient**: Optimized for continuous operation

## Hardware Requirements

- ESP32-C3 development board
- AHT10 temperature/humidity sensor
- Push button for WiFi reset
- Connecting wires
- Power supply

## Pin Configuration

| Component    | ESP32-C3 Pin |
|--------------|--------------|
| AHT10 SDA    | GPIO 8       |
| AHT10 SCL    | GPIO 9       |
| Reset Button | GPIO 2       |

## Required Libraries

Install these libraries through the Arduino IDE Library Manager:

- `WiFiManager` by tzapu
- `ArduinoJson` by Benoit Blanchon
- `HTTPClient` (included with ESP32 core)
- `Wire` (included with Arduino core)

## Configuration

Before uploading, modify these parameters in the code:

```cpp
const char* serverURL = "https://dashboard.your.domain.pl/api/data";
const String deviceID = "ESP32_AHT10_001";
const String deviceName = "Living Room Sensor";
```

## Installation

1. Install the required libraries
2. Connect the hardware according to the pin configuration
3. Update the server URL and device identifiers
4. Upload the code to your ESP32-C3
5. On first boot, connect to "ESP32-AHT10-Setup" WiFi network to configure

## Operation

- **Sensor Reading**: Every 5 seconds
- **Data Transmission**: Every 30 seconds (when valid data available)
- **WiFi Reset**: Hold reset button for 15 seconds

## Data Format

The sensor posts JSON data to the configured API endpoint:

```json
{
  "device_id": "ESP32_AHT10_001",
  "device_name": "Living Room Sensor",
  "data": {
    "temperature": 22.50,
    "humidity": 45.30,
    "rssi": -45,
    "free_heap": 234567
  }
}
```

## Troubleshooting

- **Sensor Not Found**: Check I2C connections and pin configuration
- **WiFi Issues**: Use reset button to clear settings and reconfigure
- **API Errors**: Verify server URL and network connectivity
- **Power Issues**: Ensure adequate power supply for ESP32-C3 and sensor