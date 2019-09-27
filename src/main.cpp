/*
  Desk Occupancy Manager
  Created by Richard Kingston, October 2018.
  https://github.com/kingstonrichard
*/

#include <Adafruit_NeoPixel.h>
#include <WiFiConnector.h>
#include <ESP8266HTTPClient.h> // Includes ESP8266WiFi

// Define our pins and libraries
int ledPin = D2;
int btnPin = D3;
WiFiConnector wifiConnector;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, ledPin, NEO_GRB + NEO_KHZ800);
HTTPClient httpClient;

// Environment variables
bool deskOccupied = false;
unsigned long lastStatusCheck = 0;
unsigned long statusCheckDelay = 30000;
unsigned long pulseDelay = 25;
unsigned long pulseLastMillis = 0;
int pulseVal = 0;
int pulseStep = 5;
String webServiceUri = "http://desk-occcpancy-manager.azurewebsites.net/api/desk/";
String RFID = "/000";

// We use this in various places to assign colour and brightness values to the led
void setLed(int r, int g, int b, bool resetBrightness = false) {
  pixels.setPixelColor(0, r, g, b);
  pulseVal = (resetBrightness) ? 255 : pulseVal;
  pixels.setBrightness(pulseVal);
  pixels.show();
}

// We use this to cycle the brightness of the LED up and down throughout the loop
void pulseLed() {
  if (!deskOccupied) { // Only pulse the green light
    if (pulseLastMillis == 0 || millis() - pulseLastMillis > pulseDelay) { // Limit speed
      pulseLastMillis = millis();
      pulseVal = pulseVal + pulseStep;
      if (pulseVal >= 255) { pulseStep = pulseStep - 10; pulseVal = 255; } // Change direction
      if (pulseVal <= 0) { pulseStep = pulseStep + 10; pulseVal = 0; } // Change direction
      pixels.setBrightness(pulseVal);
      pixels.show();
    }
  }
}

// Make http requests and wait for response
String callUri(String uri) {
  Serial.println(uri);
  httpClient.begin(uri);
  httpClient.GET();
  String returnVal = "200;" + httpClient.getString();
  httpClient.end();
  return returnVal;
}

// Read the desk status back from the DB and update the global variable that tracks it
void checkDesk() {
  if ((lastStatusCheck == 0) || (millis() - lastStatusCheck > statusCheckDelay)) { // Limit
    Serial.printf("Checking status of %s with DB...\n", wifiConnector.credentials.name);
    lastStatusCheck = millis();
    deskOccupied = (callUri(webServiceUri + (String)wifiConnector.credentials.name).indexOf("Occupied") > 0);
  }
}

// Write to the DB that this desk is occupied
void occupyDesk() {
  if (!deskOccupied) {  // Only do this if the desk isn't already set as occupied
    Serial.printf("Setting status of %s as occupied in DB...\n", wifiConnector.credentials.name);
    setLed(255, 0, 0, true); // Red
    deskOccupied = (callUri(webServiceUri + (String)wifiConnector.credentials.name + (String)RFID).indexOf("200;") == 0);
    lastStatusCheck = millis();
  }
}

// Write to the DB that this desk is free
void freeDesk() {
  if (deskOccupied) {  // Only do this if the desk isn't already set as free
    Serial.print("Setting status of %s as free in DB...\n");
    setLed(0, 255, 0, true); // Green
    deskOccupied = !(callUri(webServiceUri + (String)wifiConnector.credentials.name + (String)RFID).indexOf("200;") == 0);
    lastStatusCheck = millis();
  }
}

void setDesk() {
  if (!deskOccupied) {
    occupyDesk();
  } else {
    freeDesk();
  }
}

// Connect to the Wi-Fi so we can access the DB
void connectWiFi() {
  setLed(0, 0, 255); // Blue
  wifiConnector.connectWiFi();
  if (!wifiConnector.isConnected()) {
    setLed(255, 255, 255); // White.
  } else {
    setLed(0, 0, 0); // Off.
  }
}

// Setup the device on power up
void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(5000); // TODO: remove after dev - keep missing first bytes of serial
  Serial.printf("\n\nStarting up device... ");

  // Configure our pins
  pinMode(ledPin, OUTPUT);
  pinMode(btnPin, INPUT);
  pixels.begin();

  // Connect to WiFi
  Serial.println("Done!");
  connectWiFi();
}

void loop() {
  if (wifiConnector.isConnected()) {
    if (digitalRead(btnPin) == LOW) { setDesk(); }
    checkDesk();
    setLed((deskOccupied) ? 255 : 0, (deskOccupied) ? 0 : 255, 0);
    pulseLed();
    wifiConnector.closeAccessPoint();
  } else {
    wifiConnector.openAccessPoint();
  }
  wifiConnector.handleClient(); // Watch for http requests against the access point
}