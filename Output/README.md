# Output Module Documentation

This folder contains everything needed to run and assemble the **Output** mechanism of the RFID-Cocktail-Bot:  
- **Firmware** for the ESP32 (`Output.cpp`)  
- **3D models** for the output trays and screen mount  
- **Wiring & Pin Mapping** (see below)

## 1. Introduction

The **Output Module** monitors four load-cell trays to detect glass placement, displays the target drink ID on an OLED, and reports occupancy status to the server. It consists of four weight-cell assemblies and a screen mount, all wired to a single ESP32. The weight-cell assemblies are composed of the `Output_Weight_Cell_Tray.stl`, which is mounted onto the weight cell (see the Images folder). The weight cell is secured to the `Output_Weight_Cell_Lower_Part.stl` using the `Output_Weight_Cell_Spacer.stl`. The `Output_Screen_Mount.3mf` holds the OLED display, four HX711 amplifiers, and the ESP32. Each of the four weight cells connects to the ESP32 via its own HX711 amplifier. The OLED, also connected to the ESP32, displays the order number, placement status, and target drink information.

## 2. Firmware

### 2.1 File Layout

```
/rfid-cocktail-bot/Output
├── Output.cpp                       # ESP32 firmware
├── Output_Screen_Mount.3mf          # 3D model for OLED mount
├── Output_Weight_Cell_Lower_Part.stl # 3D parts for each load-cell assembly
├── Output_Weight_Cell_Spacer.stl
└── Output_Weight_Cell_Tray.stl      
```

### 2.2 Dependencies

Built with **PlatformIO**, using:

- `Arduino.h`  
- `HX711_ADC.h`  
- `EEPROM.h`  
- `WiFi.h`  
- `HTTPClient.h`  
- `U8g2lib.h`  
- `ArduinoJson.h`

### 2.3 Configuration & Constants

```cpp
// Wi-Fi
const char* ssid       = "lab@i17";
const char* password   = "lab@i17!";

// Server endpoints
const char* serverName     = "https://lehre.bpm.in.tum.de/ports/9090/weightcell";
const char* serverDisplay  = "https://lehre.bpm.in.tum.de/ports/9090/led/output";

// Load-cell pins
const int numLoadCells = 4;
const int doutPins[numLoadCells] = {27, 33, 26, 14};
const int sckPins[numLoadCells]  = {13, 32, 25, 12};

// Calibration values
float calibrationValues[numLoadCells] = {451.22, 743.72, 392.59, -118.45};

// Sensitivity threshold (grams)
float sensitivityThreshold = 5.0;

// OLED via I²C (default HW pins)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
```

### 2.4 Build & Flash

1. Open the project in PlatformIO.  
2. Upload the firmeware to an ESP32.

**Hint:** to flash the ESP32, you may have to press and hold the **BOOT** button (depending on your board's config).

### 2.5 Main Loop Behavior

- **Load-cell update**: `LoadCell[i].update()` → get weight → determine occupied vs. empty → `uploadWeightData(...)`  
- **Periodic fetch** of drink placement from `serverDisplay` → `displayDataOnOLED(...)`  
- **Delay** 100 ms per cycle

## 3. 3D-Printed Hardware

### 3.1 Parts

- **Output_Screen_Mount.3mf**: holds OLED display, four HX711 amplifiers and is used for wiring
- **Output_Weight_Cell_`parts`** (`Lower_Part`, `Spacer`, `Tray`): assemble around each load cell

### 3.2 Assembly

1. Assemble each load-cell: stack **Lower_Part**, **Spacer**, **Load Cell**, then **Tray**.  
2. Mount four amplifiers beneath the **Screen_Mount**.
3. Mount the ESP32 to the **Screen_Mount**. 
4. Screw the OLED into the mount.
5. Wire the assembly as described below.
6. Connect the weight-cell assemblies to the screen mount. 

## 4. Wiring & Pin Mapping

### 4.1 Pin-Mapping Table

| Signal                    | ESP32 Pin | Notes                  |
|---------------------------|-----------|------------------------|
| Load Cell 1 DOUT          | 27        | HX711 data pin         |
| Load Cell 1 SCK           | 13        | HX711 clock pin        |
| Load Cell 2 DOUT          | 33        |                        |
| Load Cell 2 SCK           | 32        |                        |
| Load Cell 3 DOUT          | 26        |                        |
| Load Cell 3 SCK           | 25        |                        |
| Load Cell 4 DOUT          | 14        |                        |
| Load Cell 4 SCK           | 12        |                        |
| OLED SDA (I²C)            | 21        | Default HW I²C SDA     |
| OLED SCL (I²C)            | 22        | Default HW I²C SCL     |
| USB-C Power               | USB-C     | Powers ESP32           |

### 4.2 Connector Notes

- Load cells wired via standard Dupont to HX711 modules, then to ESP32.  
- OLED uses I²C.  
- All cables are flexible jumper wires; lengths are non-critical.

## 5. Media & Demo

- **Video:** All sensor and display lines use standard Dupont cables.
- **Photos:** ESP32 is powered via USB-C.

## 6. Troubleshooting

- **No weight detection** → verify DOUT/SCK pin mapping and HX711 initialization.  
- **HTTP errors** → check `serverName` endpoint and Wi-Fi credentials.  
- **Blank OLED** → ensure correct I²C pins or pass custom pins to `u8g2` constructor.
