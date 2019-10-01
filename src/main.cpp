/*
  Desk Occupancy Manager
  Created by Richard Kingston, October 2018.
  https://github.com/kingstonrichard
*/

#include <Adafruit_NeoPixel.h>
#include <WiFiConnector.h>
#include <ESP8266HTTPClient.h> // Includes ESP8266WiFi

// Define our pins and libraries
const int ledPin = D2;
const int btnPin = D3;
WiFiConnector wifiConnector;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, ledPin, NEO_GRB + NEO_KHZ800);
HTTPClient httpClient;

// Environment variables
bool deskOccupied = false;
const char* busyText = "Occupied";
const char* freeText = "Available";
const char* apiBaseUri = "https://desk-occcpancy-manager.azurewebsites.net/api/desk";
const int rfidCode = 000;
unsigned long lastStatusCheck = 0;
unsigned long statusCheckDelay = 30000;
unsigned long pulseDelay = 25;
unsigned long pulseLastMillis = 0;
int pulseVal = 255;
int pulseStep = 5;

bool isSuccess(int responseCode) {
  return ((responseCode > 199) && (responseCode < 400));
}

const char* statusIs(bool flip = false)
{
  if (flip) {
    return (deskOccupied) ? freeText : busyText;
  }
  return (deskOccupied) ? busyText : freeText;
}

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

int makeGetRequest(int retries = 0)
{
  int responseCode = 0;
  if (wifiConnector.isConnected())
  {
    Serial.printf("Making GET request... ");
    for (int x=0; x <=retries; x++)
    {
      responseCode = httpClient.GET();
      Serial.print(httpClient.errorToString(responseCode));
      Serial.printf("[%d] ", responseCode);
      if (isSuccess(responseCode)) {
        break;
      }
      delay(2000);
    }
  }
  return responseCode;
}

// Read the current status from the DB
void getStatus()
{
  if ((lastStatusCheck == 0) || (millis() - lastStatusCheck > statusCheckDelay)) { // Limit
  
    // Prepare the request
    char apiGetUri[256];
    sprintf(apiGetUri, "%s/%s", apiBaseUri, wifiConnector.credentials.name);
    Serial.printf("Getting status of %s\n", wifiConnector.credentials.name);
    Serial.printf("Requesting %s\n", apiGetUri);
    
    // Make the request and deserialise the JSON response
    httpClient.begin(apiGetUri, "1ba408a04fe8f7b41c1b5f570900e034caec2a13");
    int responseCode = makeGetRequest(3);

    if (isSuccess(responseCode)) 
    {
      // Read the desk status from the JSON response
      Serial.printf("Processing JSON response... [");
      deskOccupied = httpClient.getString().indexOf(busyText) > 0;
      Serial.print(statusIs());
    }

    httpClient.end();
    lastStatusCheck = millis();
    Serial.printf("] Done!\n\n");
  }
}

// Write a new status into the DB
void setStatus()
{  
  // Change the light to indicate change
  setLed(0, 0, 255, true);
  
  // Prepare the request
  char apiSetUri[256];
  Serial.printf("Status is currently %s\n", statusIs());
  Serial.printf("Toggled status is therefore %s\n", statusIs(true));
  sprintf(apiSetUri, "%s/%s/%s/%ld", apiBaseUri, wifiConnector.credentials.name, statusIs(true), rfidCode);
  Serial.printf("Setting status of %s to %s for card %ld\n", wifiConnector.credentials.name, statusIs(true), rfidCode);
  Serial.printf("Requesting %s\n", apiSetUri);
  
  // Make the request to set the status in the DB
  httpClient.begin(apiSetUri, "1ba408a04fe8f7b41c1b5f570900e034caec2a13");
  int responseCode = makeGetRequest(3);
  httpClient.end();
  lastStatusCheck = millis();
  deskOccupied = !deskOccupied;
  Serial.printf("Done!\n\n");
}

// Connect to the Wi-Fi so we can access the DB
void connect() {
  Serial.printf("Setting LED to blue...");
  setLed(0, 0, 255); // Blue
  Serial.printf("Done!\n");
  wifiConnector.connectWiFi();
  if (!wifiConnector.isConnected()) {
    Serial.printf("Setting LED to white...");
    setLed(255, 255, 255); // White.
    Serial.printf("Done!\n");
  } else {
    Serial.printf("Setting LED to off...");
    setLed(0, 0, 0); // Off.
    Serial.printf("Done!\n");
  }
}

// Setup the device on power up
void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.printf("\n\nStarting up device... ");

  // Configure our pins
  pinMode(ledPin, OUTPUT);
  pinMode(btnPin, INPUT);
  pixels.begin();

  // Built in to allow us to hold button for access point
  delay(5000); // TODO: remove after dev - keep missing first bytes of serial

  // Connect to WiFi
  Serial.println("Done!");

  // If button is being held down, fire up access point instead of trying to connect
  if (digitalRead(btnPin) == LOW) 
  { 
    setLed(255, 255, 255, true);
    wifiConnector.openAccessPoint();
  } else {
    connect();
  }
}

void loop() {
  if (wifiConnector.isConnected()) {
    if (digitalRead(btnPin) == LOW) { setStatus(); }
    getStatus();
    setLed((deskOccupied) ? 255 : 0, (deskOccupied) ? 0 : 255, 0);
    pulseLed();
    wifiConnector.closeAccessPoint();
  } else {
    setLed(255, 255, 0, true);
    wifiConnector.openAccessPoint();
  }
  wifiConnector.handleClient(); // Watch for http requests against the access point
}