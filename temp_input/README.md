# Input Module Documentation

This folder contains everything needed to run and assemble the **Input** mechanism of the RFID-Cocktail-Bot:  
- **Firmware** for the ESP32 (`Input.cpp`)  
- **3D model** for the input basket (`Input_Basket.3mf`)  
- **Wiring & Pin Mapping** (see below)

## 1. Introduction

The **Input Module** handles ingredient selection via RFID tags, displays status on an OLED display, and sends button-press and tag data to the server. A SparkFun Simultaneous RFID Reader – M7E Hecto is used for scanning the RFID tags. The RFID reader is wired to an ESP32, which handles the server connection. A red and a green button are also wired to the ESP32. Pressing the green button confirms a cocktail order and sends it to the server; pressing the red button discards the order, allowing a new one to be placed. An OLED display, wired to the ESP32, shows the scanned ingredients and displays the order status and number. All components are mounted to a 3D-printed basket, which is screwed to a container, as shown in the pictures in the 'Images' folder.

## 2. Firmware

### 2.1 File Layout

```
/rfid-cocktail-bot/Input
├── Input.cpp        ← ESP32 firmware source
└── Input_Basket.3mf ← 3D model for the input basket mount
```

### 2.2 Dependencies

This sketch is built with **PlatformIO** and uses the following libaries:
- `SparkFun_UHF_RFID_Reader`  
- `WiFi`  
- `HTTPClient`  
- `Wire`  
- `U8g2lib`  
- `ArduinoJson`  
- `SoftwareSerial`

### 2.3 Configuration & Constants

```cpp
// Wi-Fi
const char* ssid         = "lab@i17";
const char* password     = "lab@i17!";

// Server endpoints
const char* serverName        = "https://lehre.bpm.in.tum.de/ports/9090/rfid_reader/tokens";
const char* serverDisplay     = "https://lehre.bpm.in.tum.de/ports/9090/led/input";
const char* buttonRedServer   = "https://lehre.bpm.in.tum.de/ports/9090/cancel";
const char* buttonGreenServer = "https://lehre.bpm.in.tum.de/ports/9090/confirm";

// Button pins
const int buttonRedPin   = 25;
const int buttonGreenPin = 26;

// RFID via SoftwareSerial
#include <SoftwareSerial.h>
SoftwareSerial softSerial(16, 17);  // RX, TX
#define rfidSerial softSerial
#define rfidBaud   38400

// OLED via I²C (default HW pins)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Upload interval (ms)
const unsigned long uploadInterval = 100;
```

### 2.4 Build & Flash

1. Open the project in PlatformIO.
2. Upload the firmeware to a ESP32.

**Hint**: to flash the ESP32, you may have to press and hold the BOOT button (depending on your board's config).

### 2.5 Main Loop Behavior

- RFID scan → parse RSSI/freq/EPC → `uploadTagData(...)`
- Periodic fetch from `serverDisplay` → `displayDataOnOLED(...)`
- Button poll → on press, `uploadButtonPress(...)` (red = cancel, green = confirm)

## 3. 3D-Printed Hardware

### 3.1 Part

- File: `Input_Basket.3mf`
- Mounting:
  - Screws to a standard plastic bucket
  - Bracket holds: RFID reader, two buttons, OLED screen, ESP32

### 3.2 Assembly

1. Screw the printed basket to the bucket lip.
2. Screw in the RFID reader, OLED, buttons, and ESP32.
3. Route cables as described below.

## 4. Wiring & Pin Mapping

### 4.1 Pin-Mapping Table

| Signal | ESP32 Pin | Notes |
|--------|-----------|-------|
| RFID RX | 16 | SoftwareSerial RX |
| RFID TX | 17 | SoftwareSerial TX |
| Button – Red | 25 | INPUT_PULLUP |
| Button – Green | 26 | INPUT_PULLUP |
| OLED SDA (I²C) | 21 | Default HW I²C SDA |
| OLED SCL (I²C) | 22 | Default HW I²C SCL |
| USB-C Power | USB-C | Powers ESP32 |

### 4.2 Connector Notes

- All sensor and display lines use standard Dupont cables.
- ESP32 is powered via USB-C.

## 5. Media & Demo

- Video: /rfid-cocktail-bot/Videos
- Photos: /rfid-cocktail-bot/Input/Images

## 6. Troubleshooting

- "Module failed to respond" → check RX/TX wiring and baud rate in `setupRfidModule()`.
- Wi-Fi doesn't connect → verify SSID/password.
- Display blank → ensure I²C pins match your board or pass custom pins to the u8g2 constructor.
