#include "AccessPoint.h"

#include <Arduino.h>
#include <WiFi.h>   //esp32

void CANServer::AccessPoint::setup(const char* ssid, const char* password)
{
    Serial.println("Setting WIFI Access Point...");
   
    WiFi.softAP(ssid, password);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    Serial.println("Done");
}