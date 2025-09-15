# BLE Heart Pulse Motion

ESP32 BLE system where a PIR motion sensor on the server triggers heartbeat LED effects on the client.

## Hardware Requirements

### Server (Motion Detection)
- ESP32 board
- PIR motion sensor

### Client (LED Display)  
- ESP32 board
- LED

## Wiring

### Server
```
ESP32 --> PIR Sensor
GND   --> GND
3.3V  --> VCC
GPIO2 --> OUT
```

### Client
```
ESP32 --> LED
GND   --> Cathode (-)
GPIO0 --> Anode (+)
```

## Setup

1. Upload `motion_sensor_server.ino` to server ESP32
2. Upload `heart_pulse_client.ino` to client ESP32
3. Power both devices

## Operation

1. Client connects to server via BLE
2. PING-PONG handshake establishes connection (LED blinks 3x)
3. Motion detection begins
4. Motion triggers heartbeat LED pattern on client
5. No motion stops heartbeat effect

## Configuration

### BLE UUIDs
- Service: `f2e8d7c5-4b3a-9e1d-8f7c-6a5b4c3d2e1f`
- Characteristic: `a1b2c3d4-5e6f-7890-abcd-ef1234567890`

### Pin Settings
- Server PIR: GPIO 2
- Client LED: GPIO 0

## Protocol Messages
- `"PING"` → Handshake initiation
- `"PONG"` → Handshake response  
- `"START"` → Motion detected
- `"STOP"` → Motion stopped

## Troubleshooting

### Connection Issues
- Check BLE range (10-30 feet)
- Verify UUIDs match
- Reset both devices

### Motion Detection
- Wait 60 seconds for PIR calibration
- Check wiring connections
- Monitor serial output

### Power Issues
- Use stable power supply
- Check for adequate current (300mA per ESP32)