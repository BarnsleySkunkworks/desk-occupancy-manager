#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
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


// Define our pins
int redLedPin = D5;
int yellowLedPin = D7;
int greenLedPin = D8;
int redBtnPin = D1;
int greenBtnPin = D2;


// Handle our desk occupancy readings
int deskOccupied = LOW;
unsigned long lastStatusCheck = 0;
unsigned long statusCheckDelay = 10000;


// Launch an access point so we can configure the device from the browser
// We might want to specify Wi-Fi settings for examples
void openAP() {
  if (!activeAP) {
    Serial.print("Launching AP...");
    digitalWrite(redLedPin, HIGH);
    digitalWrite(yellowLedPin, HIGH);
    digitalWrite(greenLedPin, HIGH);
    WiFi.mode(WIFI_AP);
    activeAP = WiFi.softAP("DOMDEVICE"); //TODO: Make this a unique id based on softAP mac address
    Serial.println("Done!");
  }
}


// Close the access point when we no longer need it, i.e. we're connected to Wi-Fi
// When we're connected to Wi-Fi the device can be configured by browsing to it's IP Address
void closeAP() {
  if (activeAP) {
    Serial.print("Closing AP...");
    activeAP = !WiFi.softAPdisconnect(true);
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, LOW);
    Serial.println("Done!");
  }
}


// Connect to the Wi-Fi so we can access the DB
void connectWiFi() {
  Serial.printf("Connecting %s to %s", creds.devId, creds.ssid);
  WiFi.disconnect(true);
  if (creds.ssid != '\0') {
    WiFi.mode(WIFI_STA);
    WiFi.hostname(creds.devId);
    digitalWrite(yellowLedPin, HIGH); // Indicates connecting
    WiFi.begin(creds.ssid, creds.pass);
    for (int x=0; x<=60; x++) { // Try to connect for up to a minute
      Serial.print(".");
      delay(500);
      if (WiFi.status() == WL_CONNECTED) { break; }
    }
    digitalWrite(yellowLedPin, LOW); // Confirms complete
  } else {
    Serial.println("Note: Cannot connect to Wi-Fi. Device has no stored credentials!");
  }
  
  // Handle based on connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Failed with status code %d", WiFi.status());
    openAP();
  } else {
    Serial.println("Done!");
    closeAP();
  }
}


void serveReset() {
  String bodyContent = "";
  if (server.hasArg("confirmed")) { // postback
    Serial.print("Handling reset postback...");
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
    bodyContent += "<input type=\"text\" class=\"textbox\" name=\"ssid\">";
    bodyContent += "<label for=\"pass\">Password</label>";
    bodyContent += "<input type=\"password\" class=\"textbox\" name=\"pass\">";
    bodyContent += "<label for=\"devId\">Device name*</label>";
    bodyContent += "<input type=\"text\" class=\"textbox\" name=\"devId\">";
    bodyContent += "<input type=\"submit\" href=\"/serve\" class=\"button\" value=\"Connect\">";
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
  if (millis() - lastStatusCheck > statusCheckDelay) {
    Serial.print("Checking desk status with DB...");
    digitalWrite(yellowLedPin, HIGH);
    lastStatusCheck = millis();
    // TODO: Go and check desk status
    digitalWrite(yellowLedPin, LOW);
    Serial.println("Done!");
  }
}


// Write to the DB that this desk is occupied
void occupyDesk() {
  if (!deskOccupied) {  // Only do this if the desk isn't already set as occupied
    Serial.print("Setting desk as occupied in DB...");
    digitalWrite(yellowLedPin, HIGH);
    // TODO: Go and update database
    deskOccupied = true;
    digitalWrite(yellowLedPin, LOW);
    Serial.println("Done!");
  }
}


// Write to the DB that this desk is free
void freeDesk() {
  if (deskOccupied) {  // Only do this if the desk isn't already set as free
    Serial.print("Setting desk as free in DB...");
    digitalWrite(yellowLedPin, HIGH);
    // TODO: Go and update database
    deskOccupied = false;
    digitalWrite(yellowLedPin, LOW);
    Serial.println("Done!");
  }
}


// Switch the LEDs appropriate to the desks availability
void setLights() {
  digitalWrite(redLedPin, deskOccupied);
  digitalWrite(greenLedPin, !deskOccupied);
}


// Setup the device on power up
void setup() {
  // Initialise our serial port for debugging
  Serial.begin(115200);
  while(!Serial);
  Serial.println(""); Serial.println("");
  delay(5000); // TODO: remove after dev - keep missing first bytes of serial
  Serial.print("Starting setup...");

  // Configure our pins
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redBtnPin, INPUT);
  pinMode(greenBtnPin, INPUT);
  digitalWrite(redLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(greenLedPin, LOW);

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
  if (digitalRead(redBtnPin) == HIGH && digitalRead(greenBtnPin) == HIGH) {
    openAP();
  } else {
    connectWiFi();
  }
}


void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    checkDesk();
    setLights();
    if (digitalRead(redBtnPin) == HIGH) { occupyDesk(); }
    if (digitalRead(greenBtnPin) == HIGH) { freeDesk(); }
    closeAP();
  } else {
    openAP();
  }
  server.handleClient();
}