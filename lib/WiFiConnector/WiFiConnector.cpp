/*
  WiFiHelper.h - Library for connecting devices to Wi-Fi.
  Presents an access point for configuration.
  Uses EEPROM to store connectivity credentials.
  Created by Richard Kingston, October 2018.
  https://github.com/kingstonrichard
*/

// TODO 1: Replace use of several Serial.print statements to get one serial output
// TODO 2: Don't use String wherever possible - lighten the overhead

#include <WiFiConnector.h>

WiFiConnector::WiFiConnector() {
  
  // Set up our webserver routes
  _server.on("/", std::bind(&WiFiConnector::handleRoot, this));
  _server.on("/reset", std::bind(&WiFiConnector::handleReset, this));
  _server.on("/configure", std::bind(&WiFiConnector::handleCapture, this));
  _server.on("/restart", std::bind(&WiFiConnector::handleRestart, this));
  _server.begin();

  // Configure our access point name
  String macAddress = "DOM-" + WiFi.softAPmacAddress();
  macAddress.replace(":", "");
  for(unsigned int x = 0; x<=macAddress.length(); x++) {
    _macAddr[x] = macAddress.charAt(x);
  }

  // Read our last known WiFi credentials from EEPROM
  EEPROM.begin(130);
  EEPROM.get(0, credentials);
}

// Launch an access point so we can configure the device from the browser
// We might want to specify Wi-Fi settings for examples
void WiFiConnector::openAccessPoint() {
  if (!_activeAP) {
    Serial.printf("Opening access point %s\n", _macAddr);
    WiFi.mode(WIFI_AP);
    _activeAP = WiFi.softAP(_macAddr);
  }
}

// Close the access point when we no longer need it, i.e. we're connected to Wi-Fi
// When we're connected to Wi-Fi the device can be configured by browsing to it's IP Address
void WiFiConnector::closeAccessPoint() {
  if (_activeAP) {
    Serial.printf("Closing access point\n");
    _activeAP = !WiFi.softAPdisconnect(true);
  }
}


void WiFiConnector::disconnectWiFi() {
  Serial.printf("Disconnecting WiFi\n");
  WiFi.disconnect(true);
}

// Connect to the Wi-Fi so we can access the DB
void WiFiConnector::connectWiFi() {
  WiFi.disconnect(true);
  if (credentials.ssid[0] != '\0') {
    Serial.printf("Connecting %s to %s\n", credentials.name, credentials.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(credentials.name);
    WiFi.begin(credentials.ssid, credentials.pass);
    for (int x=0; x<=60; x++) { // Try to connect for up to a minute
      if (WiFi.status() == WL_CONNECTED) { break; }
    }
  } else {
    Serial.printf("Cannot connect to Wi-Fi. No SSID provided\n");
  }
  
  // Handle based on connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Status code %d\n", WiFi.status());
    openAccessPoint();
  } else {
    closeAccessPoint();
  }
}

void WiFiConnector::handleClient() {
  _server.handleClient();
}

bool WiFiConnector::isConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

void WiFiConnector::handleRoot() {
  Serial.printf("Serving root\n");
  _htmlContent = "<p>SSID: " + (String)credentials.ssid + "</p>";
  _htmlContent += "<p>Device ID: " + (String)credentials.name + "</p>";
  _htmlContent += "<a href=\"/configure\" class=\"button\">Configure WiFi</a>";
  _htmlContent += "<a href=\"/reset\" class=\"button\">Factory Reset</a>";
  _htmlContent += "<a href=\"/restart\" class=\"button\">Restart Device</a>";
  serveContent("Desk Occupancy");
}

void WiFiConnector::handleReset() {
  if (_server.hasArg("confirmed")) { // postback
    Serial.printf("Handling reset postback\n");
    memset(credentials.ssid, 0, sizeof credentials.ssid);
    memset(credentials.pass, 0, sizeof credentials.pass);
    memset(credentials.name, 0, sizeof credentials.name);
    credentials = { '\0', '\0', '\0' };
    EEPROM.put(0, credentials);
    EEPROM.commit();
    _htmlContent = "<p>Settings have been reset. A restart is required to apply them.</p>";
    _htmlContent += "<a href=\"/restart\" class=\"button\">Restart</a>";
    _htmlContent += "<a href=\"/\" class=\"button\">Back</a>";
  } else { // no postback
    Serial.printf("Serving reset page\n");
    _htmlContent = "<p>Are you sure you want to wipe all settings?</p>";
    _htmlContent += "<a href=\"/reset?confirmed=true\" class=\"button\">Yes</a>";
    _htmlContent += "<a href=\"/\" class=\"button\">No</a>";
  }
  serveContent("Reset device");
}

// If a client does want the configuration page, display it and handle any postbacks
void WiFiConnector::handleCapture() {
  if ((_server.hasArg("ssid")) && (_server.hasArg("pass")) && (_server.hasArg("name"))) { // postback
    Serial.print("Handling configuration postback...");
    _server.arg("ssid").toCharArray(credentials.ssid, 33);
    _server.arg("pass").toCharArray(credentials.pass, 64);
    _server.arg("name").toCharArray(credentials.name, 33);
    EEPROM.put(0, credentials);
    EEPROM.commit();
    _htmlContent = "<p>Configuration settings saved. Restart the device to apply them.</p>";
    _htmlContent += "<a href=\"/restart\" class=\"button\">Restart</a>";
    _htmlContent += "<a href=\"/\" class=\"button\">Back</a>";
  } else { // no postback
    Serial.print("Serving configuration page...");
    _htmlContent = "<form action=\"/configure\" method=\"post\" onSubmit=\"validateForm()\">";
    _htmlContent += "<label for=\"ssid\">SSID*</label>";
    _htmlContent += "<input type=\"text\" class=\"textbox\" name=\"ssid\" value=\"" + (String)credentials.ssid + "\">";
    _htmlContent += "<label for=\"pass\">Password</label>";
    _htmlContent += "<input type=\"password\" class=\"textbox\" name=\"pass\" value=\"" + (String)credentials.pass + "\">";
    _htmlContent += "<label for=\"name\">Device name*</label>";
    _htmlContent += "<input type=\"text\" class=\"textbox\" name=\"name\" value=\"" + (String)credentials.name + "\">";
    _htmlContent += "<input type=\"submit\" href=\"/serve\" class=\"button\" value=\"Save\">";
    _htmlContent += "</form>";
    _htmlContent += "<a href=\"/\" class=\"button\">Back</a>";
  }
  serveContent("Configure device");
  Serial.println("Done!");
}

void WiFiConnector::handleRestart() {  
  if (_server.hasArg("confirmed")) { // postback
    Serial.print("Restarting device...");
    _htmlContent = "<p>Device is being restarted. You may have to reconnect with it.</p>";
    _htmlContent += "<a href=\"/\" class=\"button\">Back</a>";
  } else { // no postback
    Serial.print("Serving restart page...");
    _htmlContent = "<p>Are you sure you want to restart the device?</p>";
    _htmlContent += "<a href=\"/restart?confirmed=true\" class=\"button\">Yes</a>";
    _htmlContent += "<a href=\"/\" class=\"button\">No</a>";
  }
  serveContent("Restart device");
  if (_server.hasArg("confirmed")) {
    delay(3000); // to give time to return the page to the client
    ESP.restart(); // TODO: Prevent the watchdog timeout causing a restart too.
  }
}

// Compile a HTML template from PageTemplate.cpp and our title and 
// content parameters, then push it out to the client
void WiFiConnector::serveContent(String title) {
  String pageContent = _HTMLPage;
  pageContent.replace("{{pageTitle}}", title);
  pageContent.replace("{{pageContent}}", _htmlContent);
  _server.send(200, "text/html", pageContent);
}