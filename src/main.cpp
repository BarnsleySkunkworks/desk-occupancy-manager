#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


// Setup our WiFi configuration
const char* ssid = "GetYourAssToMars";
const char* password = "O2caR&Jb#2013&17";
const String deviceId = "DOM-GP-L9-001";
ESP8266WebServer server(80);


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


void checkDesk() {
  digitalWrite(yellowLedPin, HIGH);
  if (millis() - lastStatusCheck > statusCheckDelay) {  // Don't check too often - stick to the plan
    lastStatusCheck = millis();
    // TODO: Go and check desk status
  }
  digitalWrite(yellowLedPin, LOW);
}


void occupyDesk() {
  if (!deskOccupied) {  // Only do this if the desk isn't already occupied
    digitalWrite(yellowLedPin, HIGH);
    // TODO: Go and update database
    deskOccupied = HIGH;
    digitalWrite(yellowLedPin, LOW);
  }
}


void freeDesk() {
  if (deskOccupied) {  // Only do this if the desk isn't already free
    digitalWrite(yellowLedPin, HIGH);
    // TODO: Go and update database
    deskOccupied = LOW;
    digitalWrite(yellowLedPin, LOW);
  }
}


void serveConfigurePage() {
  String configurePage = "<!DOCTYPE HTML><html><head><title>Desk Occupancy: Configure Device</title></head><body>";
  configurePage += "<p>Coming soon...</p></body></html>";
  server.send(200, "text/html", configurePage);  
}


void serveHomePage() {
  String homePage = "<!DOCTYPE HTML><html><head><title>Desk Occupancy</title></head><body>";
  homePage += "<h1>Desk: " + deviceId + "</h1><p>Status: ";
  homePage += (deskOccupied == 1) ? "Occupied" : "Available";
  homePage += "</p></body></html>";
  server.send(200, "text/html", homePage);
}


void setLights() {
  digitalWrite(redLedPin, deskOccupied);
  digitalWrite(greenLedPin, !deskOccupied);
}


void setup() {
  // Initialise our serial port for debugging
  Serial.begin(115200);
  while(!Serial);
 
  // Configure our pins
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redBtnPin, INPUT);
  pinMode(greenBtnPin, INPUT);
  digitalWrite(redLedPin, LOW);
  digitalWrite(yellowLedPin, HIGH); // To indicate connecting
  digitalWrite(greenLedPin, LOW);
 
  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname(deviceId);
  WiFi.begin(ssid, password);

  //Trying to connect it will display dots
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Once connected, fire up the web server and indicate success
  server.on("/", serveHomePage);
  server.on("/configure", serveConfigurePage);
  server.begin();
  digitalWrite(yellowLedPin, LOW); // To visually confirm connected and server is up

  // Output to serial so we can see what's going on during debugging
  Serial.println("");
  Serial.print("WiFi connected at http://");
  Serial.print(WiFi.localIP());
  Serial.print(" or http://");
  Serial.println(deviceId);

  // Check status of this desk
  checkDesk();
}


void loop() {
  if (digitalRead(redBtnPin) == HIGH) { occupyDesk(); }
  if (digitalRead(greenBtnPin) == HIGH) { freeDesk(); }
  setLights();
  server.handleClient();
}
