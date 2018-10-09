/*
  WiFiConnector.h - Library for connecting devices to Wi-Fi.
  Presents an access point for configuration.
  Uses EEPROM to store connectivity credentials.
  Created by Richard Kingston, October 2018.
  https://github.com/kingstonrichard
*/

#ifndef WiFiConnector_h
#define WiFiConnector_h

#include <ESP8266WebServer.h> // Includes ESP8266WiFi
#include <EEPROM.h>
#include "PageTemplate.cpp"

class WiFiConnector
{    
  public:
    WiFiConnector();
    void openAccessPoint();
    void closeAccessPoint();
    void connectWiFi();
    void disconnectWiFi();
    void handleClient();
    bool isConnected();
    struct EEPROMData { // to hold device settings
      char ssid[33]; //32 max allowed + 1 for the terminating null
      char pass[64]; //63 max allowed + 1
      char name[33]; // 32 max allowed + 1
    } credentials;
  private:
    void handleRoot();
    void handleReset();
    void handleCapture();
    void handleRestart();
    void serveContent(String content);
    bool _activeAP;
    char _macAddr[16];
    String _htmlContent;
    ESP8266WebServer _server;
};

#endif