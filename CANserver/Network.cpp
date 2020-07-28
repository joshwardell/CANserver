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

void WiFiEvent(WiFiEvent_t event)
{
    switch(event) 
    {
        case SYSTEM_EVENT_AP_START:
            //can set ap hostname here
            WiFi.softAPsetHostname(displaySSID);
            break;
        case SYSTEM_EVENT_STA_START:
            //set sta hostname here
            WiFi.setHostname(displaySSID);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Connected to external WiFI network (");
            Serial.print(_externalSSID);
            Serial.print("): ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            //Should we try and re-connect?
            break;
        default:
            break;
    }
}

void CANServer::Network::setup()
{
    Serial.println("Setting up Networking ...");

    Serial.print("WiFi MAC: ");
    Serial.println(WiFi.macAddress());

    _networkingPrefs.begin("Networking");

    pinMode(CFG1,INPUT_PULLUP);
    if (digitalRead(CFG1) == 0) { //If jumpered, server 2
        displaySSID = "CANserver2";
    }

    //Based on https://github.com/espressif/arduino-esp32/issues/2537
    //we need to do the following crazyness to make sure the hostname is set correctly
    WiFi.disconnect(true);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(displaySSID);

    WiFi.onEvent(WiFiEvent);

    //Lets see if we have wireless credentials to connect to
    _externalSSID = _networkingPrefs.getString("externalSSID", "");
    _externalPw = _networkingPrefs.getString("externalPw", "");
    if (_externalSSID.length() > 0)
    {
        //We have an external SSID configuration.  Setup as WIFI AP/STA mode
        Serial.print("Connecting to external WiFi: ");
        Serial.println(_externalSSID);

        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(_externalSSID.c_str(), _externalPw.c_str());
    }

    //Now start up the Soft AP for the displays to connect to
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