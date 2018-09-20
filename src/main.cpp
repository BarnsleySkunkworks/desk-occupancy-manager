#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include "../lib/DOMPages/HTMLPage.h"


// Setup our WiFi variables
struct EEPROMData { // to hold device settings
  char ssid[33]; //32 max allowed + 1 for the terminating null
  char pass[64]; //63 max allowed + 1
  char devId[33]; // 32 max allowed + 1
};
EEPROMData creds = { '\0', '\0', '\0' };
ESP8266WebServer server(80);
bool activeAP = false;
HTTPClient http;
char macAddr[16];

// Define our pins
int ledPin = D2;
int btnPin = D3;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, ledPin, NEO_GRB + NEO_KHZ800);

// Handle our desk occupancy readings
bool deskOccupied = false;
unsigned long lastStatusCheck = 0;
unsigned long statusCheckDelay = 30000;

// Light the NeoPixel as we need
void setLed(int r, int g, int b) {
  pixels.setPixelColor(0, r, g, b);
  pixels.show();
}

// Launch an access point so we can configure the device from the browser
// We might want to specify Wi-Fi settings for examples
void openAP() {
  if (!activeAP) {
    Serial.print("Launching AP ");
    Serial.print(macAddr);
    Serial.print("...");
    setLed(200,162,200); // White
    WiFi.mode(WIFI_AP);
    activeAP = WiFi.softAP(macAddr);
    Serial.println("Done!");
  }
}


// Close the access point when we no longer need it, i.e. we're connected to Wi-Fi
// When we're connected to Wi-Fi the device can be configured by browsing to it's IP Address
void closeAP() {
  if (activeAP) {
    Serial.print("Closing AP...");
    activeAP = !WiFi.softAPdisconnect(true);
    setLed(0, 0, 0); // Off
    Serial.println("Done!");
  }
}


// Connect to the Wi-Fi so we can access the DB
void connectWiFi() {
  WiFi.disconnect(true);
  if (creds.ssid[0] != '\0') {
    Serial.printf("Connecting %s to %s", creds.devId, creds.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(creds.devId);
    setLed(0, 0, 255); // Blue
    WiFi.begin(creds.ssid, creds.pass);
    for (int x=0; x<=60; x++) { // Try to connect for up to a minute
      Serial.print(".");
      delay(500);
      if (WiFi.status() == WL_CONNECTED) { break; }
    }
    setLed(0, 0, 0); // Off.
  } else {
    Serial.print("Cannot connect to Wi-Fi. Device has no stored credentials. ");
  }
  
  // Handle based on connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Status code %d!\n", WiFi.status());
    openAP();
  } else {
    Serial.println("Done!");
    http.begin("http://desk-occcpancy-manager.azurewebsites.net/api/desk/register/" + (String)creds.devId);
    http.GET();
    http.end();
    closeAP();
  }
}


void serveReset() {
  String bodyContent = "";
  if (server.hasArg("confirmed")) { // postback
    Serial.print("Handling reset postback...");
    memset(creds.ssid, 0, sizeof creds.ssid);
    memset(creds.pass, 0, sizeof creds.pass);
    memset(creds.devId, 0, sizeof creds.devId);
    creds = { '\0', '\0', '\0' };
    EEPROM.put(0, creds);
    EEPROM.commit();
    bodyContent += "<p>Settings have been reset. A restart is required to apply them.</p>";
    bodyContent += "<a href=\"/restart\" class=\"button\">Restart</a>";
    bodyContent += "<a href=\"/\" class=\"button\">Back</a>";
  } else { // no postback
    Serial.print("Serving reset page...");
    bodyContent += "<p>Are you sure you want to wipe all settings?</p>";
    bodyContent += "<a href=\"/reset?confirmed=true\" class=\"button\">Yes</a>";
    bodyContent += "<a href=\"/\" class=\"button\">No</a>";
  }

  // Compile page and send to client
  String pageContent = HTMLPage;
  pageContent.replace("{{pageTitle}}", "Reset device");
  pageContent.replace("{{pageContent}}", bodyContent);
  server.send(200, "text/html", pageContent);
  Serial.println("Done!");
}


// If a client does want the configuration page, display it and handle any postbacks
void serveConfigure() {
  String bodyContent = "";
  if ((server.hasArg("ssid")) && (server.hasArg("pass")) && (server.hasArg("devId"))) { // postback
    Serial.print("Handling configuration postback...");
    server.arg("ssid").toCharArray(creds.ssid, 33);
    server.arg("pass").toCharArray(creds.pass, 64);
    server.arg("devId").toCharArray(creds.devId, 33);
    EEPROM.put(0, creds);
    EEPROM.commit();
    bodyContent += "<p>Configuration settings saved. Restart the device to apply them.</p>";
    bodyContent += "<a href=\"/restart\" class=\"button\">Restart</a>";
    bodyContent += "<a href=\"/\" class=\"button\">Back</a>";
  } else { // no postback
    Serial.print("Serving configuration page...");
    bodyContent += "<form action=\"/configure\" method=\"post\" onSubmit=\"validateForm()\">";
    bodyContent += "<label for=\"ssid\">SSID*</label>";
    bodyContent += "<input type=\"text\" class=\"textbox\" name=\"ssid\" value=\"" + (String)creds.ssid + "\">";
    bodyContent += "<label for=\"pass\">Password</label>";
    bodyContent += "<input type=\"password\" class=\"textbox\" name=\"pass\" value=\"" + (String)creds.pass + "\">";
    bodyContent += "<label for=\"devId\">Device name*</label>";
    bodyContent += "<input type=\"text\" class=\"textbox\" name=\"devId\" value=\"" + (String)creds.devId + "\">";
    bodyContent += "<input type=\"submit\" href=\"/serve\" class=\"button\" value=\"Save\">";
    bodyContent += "</form>";
    bodyContent += "<a href=\"/\" class=\"button\">Back</a>";
  }

  // Compile page and send to client
  String pageContent = HTMLPage;
  pageContent.replace("{{pageTitle}}", "Configure device");
  pageContent.replace("{{pageContent}}", bodyContent);
  server.send(200, "text/html", pageContent);
  Serial.println("Done!");
}


void serveRestart() {  
  String bodyContent = "";
  if (server.hasArg("confirmed")) { // postback
    Serial.print("Restarting device...");
    bodyContent += "<p>Device is being restarted. You may have to reconnect with it.</p>";
    bodyContent += "<a href=\"/\" class=\"button\">Back</a>";
  } else { // no postback
    Serial.print("Serving restart page...");
    bodyContent += "<p>Are you sure you want to restart the device?</p>";
    bodyContent += "<a href=\"/restart?confirmed=true\" class=\"button\">Yes</a>";
    bodyContent += "<a href=\"/\" class=\"button\">No</a>";
  }
  
  // Compile page and send to client
  String pageContent = HTMLPage;
  pageContent.replace("{{pageTitle}}", "Restart device");
  pageContent.replace("{{pageContent}}", bodyContent);
  server.send(200, "text/html", pageContent);

  // Restart if confirmed  delay(3000);
  if (server.hasArg("confirmed")) {
    delay(3000); // to give time to return the page to the client
    ESP.restart(); // TODO: Prevent the watchdog timeout causing a restart too.
  }
}


void serveHome() {
  Serial.print("Serving home page...");
  String bodyContent = "";
  bodyContent += "<p>SSID: " + (String)creds.ssid + "</p>";
  bodyContent += "<p>Device ID: " + (String)creds.devId + "</p>";
  bodyContent += "<a href=\"/configure\" class=\"button\">Configure WiFi</a>";
  bodyContent += "<a href=\"/reset\" class=\"button\">Factory Reset</a>";
  bodyContent += "<a href=\"/restart\" class=\"button\">Restart Device</a>";
  
  // Compile page and send to client
  String pageContent = HTMLPage;
  pageContent.replace("{{pageTitle}}", "Desk Occupancy");
  pageContent.replace("{{pageContent}}", bodyContent);
  server.send(200, "text/html", pageContent);
  Serial.println("Done!");
}


// Read the desk status back from the DB
void checkDesk() {
  if ((lastStatusCheck ==0) || (millis() - lastStatusCheck > statusCheckDelay)) {
    Serial.print("Checking status of ");
    Serial.print(creds.devId);
    Serial.print(" with DB...");
    lastStatusCheck = millis();
    http.begin("http://desk-occcpancy-manager.azurewebsites.net/api/desk/" + (String)creds.devId);
    int xyz = 0;
    while (xyz != 200) {
      xyz = http.GET();
    }
    
    String payload = http.getString();
    if (payload.indexOf("status") > 0) {
      Serial.print(" received ");
      Serial.print(payload);
      Serial.println("!");
      deskOccupied = (payload.indexOf("Occupied") > 0);
    }
    http.end();
  }
}



// Write to the DB that this desk is occupied
void occupyDesk() {
  if (!deskOccupied) {  // Only do this if the desk isn't already set as occupied
    Serial.print("Setting desk as occupied in DB...");
    setLed(255, 0, 0); // Red
    http.begin("http://desk-occcpancy-manager.azurewebsites.net/api/desk/" + (String)creds.devId + "/occupied");
    if (http.GET() == 200) { deskOccupied = true; }
    http.end();
    lastStatusCheck = millis();
    Serial.println("Done!");
  }
}


// Write to the DB that this desk is free
void freeDesk() {
  if (deskOccupied) {  // Only do this if the desk isn't already set as free
    Serial.print("Setting desk as free in DB...");
    setLed(0, 255, 0); // Green
    http.begin("http://desk-occcpancy-manager.azurewebsites.net/api/desk/" + (String)creds.devId + "/available");
    if (http.GET() == 200) { deskOccupied = false; }
    http.end();
    lastStatusCheck = millis();
    Serial.println("Done!");
  }
}

// Setup the device on power up
void setup() {
  // Initialise our serial port for debugging
  Serial.begin(115200);
  while(!Serial);
  Serial.println(""); Serial.println("");
  delay(5000); // TODO: remove after dev - keep missing first bytes of serial
  Serial.print("Starting setup of device ");

  // Get our macAddress in case we have to launch an AP
  String macAddress = "DOM-" + WiFi.softAPmacAddress();
  macAddress.replace(":", "");
  Serial.print(macAddress);
  Serial.print("...");
  for(int x = 0; x<=macAddress.length(); x++) {
    macAddr[x] = macAddress.charAt(x);
  }

  // Configure our pins
  pinMode(ledPin, OUTPUT);
  pinMode(btnPin, INPUT);
  pixels.begin();

  // Setup our AP in case remote config is required
  server.on("/", serveHome);
  server.on("/reset", serveReset);
  server.on("/configure", serveConfigure);
  server.on("/restart", serveRestart);
  server.begin();
 
  // Read our last known WiFi credentials from EEPROM
  EEPROM.begin(130);
  EEPROM.get(0, creds);
  Serial.println("Done!");

  // Connect to Wi-Fi now everythig is prepared
  if (digitalRead(btnPin) == LOW) {
    openAP();
  } else {
    connectWiFi();
  }
}


void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (digitalRead(btnPin) == LOW) {
      if (!deskOccupied) {
        occupyDesk();
      } else {
        freeDesk();
      }
    }
    checkDesk();
    setLed((deskOccupied) ? 255 : 0, (deskOccupied) ? 0 : 255, 0);
    closeAP();
  } else {
    openAP();
  }
  server.handleClient();
}