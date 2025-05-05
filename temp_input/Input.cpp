#include <Arduino.h> 
#include "SparkFun_UHF_RFID_Reader.h" // Library for controlling the RFID module
#include "WiFi.h" //sending Data via WiFi
#include <HTTPClient.h> //sending Data via HTTP
#include <Wire.h> 
#include <U8g2lib.h> // U8g2 library for the display
#include <ArduinoJson.h> // ArduinoJson library for parsing JSON

const char* ssid = "lab@i17"; 
const char* password = "lab@i17!"; 
const char* serverName = "https://lehre.bpm.in.tum.de/ports/9090/rfid_reader/tokens"; //server adress
const char* serverDisplay = "https://lehre.bpm.in.tum.de/ports/9090/led/input"; //server adress
const char* buttonRedServer = "https://lehre.bpm.in.tum.de/ports/9090/cancel"; //server adress
const char* buttonGreenServer = "https://lehre.bpm.in.tum.de/ports/9090/confirm"; //server adress

// Button pins
const int buttonRedPin = 25;
const int buttonGreenPin = 26;

// Variables for uploading to server
String tagsArray[10]; // Array to store tag data
int tagCount = 0;     // Counter for tags in array
unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 100; // Upload every 0.1 seconds to display

// Create instance of the RFID module
RFID rfidModule;

// We use softeare serial to connect the RFID module to the ESP32
#include <SoftwareSerial.h>
SoftwareSerial softSerial(16, 17); //RX, TX; Here you can specify which serial port the RFID module is connected to.
#define rfidSerial softSerial // We use software serial for the connection
#define rfidBaud 38400 // Baud rate for the module. 38400 is recommended if using software serial, and 115200 if using hardware serial.

// Initialize U8g2 display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// Here you can select which module you are using. This library was originally
// written for the M6E Nano only, and that is the default if the module is not
// specified. Support for the M7E Hecto has since been added, which can be
// selected below
// #define moduleType ThingMagic_M6E_NANO
#define moduleType ThingMagic_M7E_HECTO

// HTTPClient instance
HTTPClient http;

boolean setupRfidModule(long baudRate);

// Function to handle HTTP uploads
void uploadTagData(String tagData) {
  if(WiFi.status() == WL_CONNECTED) {
      // Server endpoint
  http.begin(serverName);
    
  // Setting content type header
  http.addHeader("Content-Type", "application/json");
        
    // Create JSON payload
    String httpRequestData = "{\"tag_data\": \"" + tagData + "\"}";
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// Function to handle button press uploads
void uploadButtonPress(const char* server) {
  if(WiFi.status() == WL_CONNECTED) {

    // Server endpoint
    http.begin(server);
      
    // Setting content type header
    http.addHeader("Content-Type", "application/json");
    
  // Setting content type header
  http.addHeader("Content-Type", "application/json");
      
    // Create JSON payload
    String httpRequestData = "{\"button_press\": \"true\"}";
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
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

  // Extract data from JSON
  const char* ingredients = doc["ingredients"];
  const char* order_number = doc["order_number"];
  //const char* drink_placement = doc["drink_placement"];

  // Display strings with wrapping
  int lineHeight = 12; // Adjust line height as needed
  int maxWidth = 128; // Width of the display
  int cursorY = 15; // Starting Y position

  auto displayString = [&](const char* str) {
    String line = "";
    for (int i = 0; i < strlen(str); i++) {
      line += str[i];
      int lineWidth = u8g2.getStrWidth(line.c_str());
      if (lineWidth > maxWidth || str[i] == '\n') {
        u8g2.drawStr(0, cursorY, line.c_str());
        cursorY += lineHeight;
        line = "";
      }
    }
    if (line.length() > 0) {
      u8g2.drawStr(0, cursorY, line.c_str());
      cursorY += lineHeight;
    }
  };

  displayString(ingredients);
  displayString(order_number);
  //displayString(drink_placement);

  u8g2.sendBuffer();
}

// Function to fetch data from the server and display it on the OLED
void fetchDataAndDisplay() {
  if(WiFi.status() == WL_CONNECTED) {
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
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void setup()
{
  Serial.begin(115200);

  // Initialize OLED display
  u8g2.begin();

  // Initialize buttons
  pinMode(buttonRedPin, INPUT_PULLUP);
  pinMode(buttonGreenPin, INPUT_PULLUP);

  WiFi.begin(ssid, password); //Set WiFi to station mode and disconnect from an AP if it was previously connected.
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  
  Serial.println("Connected to the WiFi network");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  //delay(10000); // Delay to allow WiFi to connect - try if module does not respond

  // Wait for the serial port to come online with a timeout
  unsigned long startMillis = millis();
  while (!Serial && (millis() - startMillis < 5000)) {
    // Wait for up to 5 seconds for the serial port to become available
  }

  if (setupRfidModule(rfidBaud) == false)
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  rfidModule.setRegion(REGION_NORTHAMERICA); //Set to North America

  rfidModule.setReadPower(1200); //100 = 1.00 dBm. Higher values than 2000 may caues USB port to brown out according to documentation
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  Serial.println(F("Scanning starts now"));

  rfidModule.startReading(); //Begin scanning for tags
}

void loop() {
  if (rfidModule.check() == true) {
    byte responseType = rfidModule.parseResponse();

    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
      Serial.println(F("Scanning"));
    }
    else if (responseType == RESPONSE_IS_TAGFOUND) {
      // Process tag data
      int rssi = rfidModule.getTagRSSI();
            // Print to serial
            Serial.print(F(" test_rssi[")); 
            Serial.print(rssi);
            // ... rest of serial print code
      long freq = rfidModule.getTagFreq();
      long timeStamp = rfidModule.getTagTimestamp();
      byte tagEPCBytes = rfidModule.getTagEPCBytes();

      // Create a string with the tag data
      char tagData[128];
      snprintf(tagData, sizeof(tagData), "RSSI:%d,FREQ:%ld,TIME:%ld,EPC:", rssi, freq, timeStamp);
         
         // Add EPC bytes to the tag data
         for (byte x = 0; x < tagEPCBytes; x++) {
           if (rfidModule.msg[31 + x] < 0x10) strcat(tagData, "0");
           char hexByte[3];
           snprintf(hexByte, sizeof(hexByte), "%X ", rfidModule.msg[31 + x]);
           strcat(tagData, hexByte);
         }
         Serial.println();
      // Upload the tag data immediately
      uploadTagData(tagData);
      
      // Print to serial
      Serial.print(F(" test_rssi[")); 
      Serial.print(rssi);
      // ... rest of serial print code
    }
    // ... rest of response handling
  }
    // Fetch data from the server and display it on the OLED every XX seconds
    if (millis() - lastUploadTime >= uploadInterval) {
      lastUploadTime = millis();
      fetchDataAndDisplay();
    }

    // Check button states and upload button press data
    if (digitalRead(buttonRedPin) == LOW) {
      uploadButtonPress(buttonRedServer);
      delay(100); // Debounce delay
    }
    if (digitalRead(buttonGreenPin) == LOW) {
      uploadButtonPress(buttonGreenServer);
      delay(100); // Debounce delay //smaller delay works better -> maybe try for fetch data
    }
}

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupRfidModule(long baudRate)
{
  rfidModule.begin(rfidSerial, moduleType); //Tell the library to communicate over serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  rfidSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  delay(100); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while (rfidSerial.available())
    rfidSerial.read();

  rfidModule.getVersion();

  if (rfidModule.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    rfidModule.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    rfidSerial.begin(115200); //Start serial at 115200

    rfidModule.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    rfidSerial.begin(baudRate); //Start the serial port, this time at user's chosen baud rate

    delay(250);
  }

  //Test the connection
  rfidModule.getVersion();
  if (rfidModule.msg[0] != ALL_GOOD)
    return false; //Something is not right

  //The module has these settings no matter what
  rfidModule.setTagProtocol(); //Set protocol to GEN2

  rfidModule.setAntennaPort(); //Set TX/RX antenna ports to 1

  return true; //We are ready to rock
}

