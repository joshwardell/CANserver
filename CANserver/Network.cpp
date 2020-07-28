#include "Network.h"

#include <Arduino.h>
#include <WiFi.h>   //esp32
#include <Preferences.h>

// access point network credentials - don't change these
const char* displaySSID = "CANserver";
const char* displayPassword = "JWcanServer2020";

#define CFG1 15   //jumper to toggle second CANserver

Preferences _networkingPrefs;

String _externalSSID = "";
String _externalPw = "";

void CANServer::Network::setup()
{
    Serial.println("Setting Networking ...");

    Serial.print("WiFi MAC: ");
    Serial.println(WiFi.macAddress());

    _networkingPrefs.begin("Networking");

    //Lets see if we have wireless credentials to connect to
    _externalSSID = _networkingPrefs.getString("externalSSID", "");
    _externalPw = _networkingPrefs.getString("externalPw", "");
    if (_externalSSID.length() > 0)
    {
        //We have an external SSID configuration.  Setup as WIFI AP/STA mode
        Serial.print("Connection to external WiFi: ");
        Serial.println(_externalSSID);

        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(_externalSSID.c_str(), _externalPw.c_str());
    }

    pinMode(CFG1,INPUT_PULLUP);
    if (digitalRead(CFG1) == 0) { //If jumpered, server 2
        displaySSID = "CANserver2";
    }
    

    WiFi.softAP(displaySSID, displayPassword);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Soft AP IP address: ");
    Serial.println(IP);

    //Make some changes to the DHCP server configuration so we don't serve a DNS server or Router to clients
    //This helps phones not loose their wider network when connected to the CANServer's wifi
    tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
    uint8_t opt_val = 0; // don't supply a dns server via dhcps
    tcpip_adapter_dhcps_option(TCPIP_ADAPTER_OP_SET, TCPIP_ADAPTER_DOMAIN_NAME_SERVER, &opt_val, 1);  // don't supply a dns server via dhcps
	tcpip_adapter_dhcps_option(TCPIP_ADAPTER_OP_SET, TCPIP_ADAPTER_ROUTER_SOLICITATION_ADDRESS, &opt_val, 1);  // don't supply a gateway (router) via dhcps option 3

    tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);

    Serial.println("Done");
}

void CANServer::Network::handle()
{

}


void CANServer::Network::setExternalWifiSSID(const String value)
{
    _externalSSID = value;
    _networkingPrefs.putString("externalSSID", _externalSSID);
}

void CANServer::Network::setExternalWifiPassword(const String value)
{
    _externalPw = value;
    _networkingPrefs.putString("externalPw", _externalPw);
}

const char* CANServer::Network::getExternalWifiSSID()
{
    return _externalSSID.c_str();
}