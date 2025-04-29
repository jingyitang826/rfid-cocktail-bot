#include <Arduino.h>
#include <HX711_ADC.h> // HX711 library for load cell
#include <EEPROM.h> // EEPROM library for storing calibration values
#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h> // U8g2 library for the display
#include <ArduinoJson.h> // ArduinoJson library for parsing JSON

// WiFi settings
const char* ssid = "lab@i17"; 
const char* password = "lab@i17!";  
const char* serverName = "https://lehre.bpm.in.tum.de/ports/9090/weightcell";
const char* serverDisplay = "https://lehre.bpm.in.tum.de/ports/9090/led/output"; 

// Weight cell pins
const int numLoadCells = 4;
const int doutPins[numLoadCells] = {27, 33, 26, 14};
const int sckPins[numLoadCells] = {13, 32, 25, 12};

// HX711 instances
HX711_ADC LoadCell[numLoadCells] = {
    HX711_ADC(doutPins[0], sckPins[0]),
    HX711_ADC(doutPins[1], sckPins[1]),
    HX711_ADC(doutPins[2], sckPins[2]),
    HX711_ADC(doutPins[3], sckPins[3])
};

// Calibration values
float calibrationValues[numLoadCells] = {451.22, 743.72, 392.59, -118.45};

// Sensitivity threshold
float sensitivityThreshold = 5.0;

// Previous weight states
bool previousStates[numLoadCells] = {false, false, false, false};

// Initialize U8g2 display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void uploadWeightData(const String& payload) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverName);
        http.addHeader("Content-Type", "application/json");

        int httpResponseCode = http.POST(payload);

        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
}

// Function to display data on the OLED
void displayDataOnOLED(const String& data) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    // Parse JSON data
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
        u8g2.setCursor(0, 15);
        u8g2.print("JSON Parse Error");
        u8g2.sendBuffer();
        return;
    }

    // Extract order number from JSON
    const char* drink_placement = doc["drink_placement"];

    // Display order number
    u8g2.setCursor(0, 15);
    u8g2.print("Drink Placement: ");
    u8g2.setCursor(0, 30);
    u8g2.print(drink_placement);
    u8g2.sendBuffer();
}

// Function to fetch data from the server and display it on the OLED
void fetchDataAndDisplay() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverDisplay); // Use the updated serverDisplay variable
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
            String payload = http.getString();
            Serial.println("Server response: " + payload);

            // Display data on OLED
            displayDataOnOLED(payload);
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();

    // Initialize OLED display
    u8g2.begin();

    for (int i = 0; i < numLoadCells; i++) {
        LoadCell[i].begin();
        LoadCell[i].start(2000, true);
        LoadCell[i].setCalFactor(calibrationValues[i]);
    }
}

void loop() {
    for (int i = 0; i < numLoadCells; i++) {
        LoadCell[i].update();
        float weight = LoadCell[i].getData();
        bool currentState = weight > sensitivityThreshold;

        if (currentState != previousStates[i]) {
            previousStates[i] = currentState;
            String payload = "{\"LoadCell\": " + String(i) + ", \"status\": \"" + (currentState ? "Occupied" : "Empty") + "\"}";
            uploadWeightData(payload);
            Serial.println(payload);
        }
    }

    // Fetch data from the server and display it on the OLED every 5 seconds
    static unsigned long lastFetchTime = 0;
    if (millis() - lastFetchTime >= 5000) {
        lastFetchTime = millis();
        fetchDataAndDisplay();
    }

    delay(100);
}