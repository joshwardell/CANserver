#include "Network.h"

#include <Arduino.h>
#include <WiFi.h>   //esp32
#include <esp_wifi.h>
#include <Preferences.h>

// access point network credentials - don't change these
const char* displaySSID = "CANserver";
const char* displayPassword = "JWcanServer2020";

#define CFG1 15   //jumper to toggle second CANserver

Preferences _networkingPrefs;

String _externalSSID = "";
String _externalPw = "";


#define MIN_SCAN_CHANNEL 1
#define MAX_SCAN_CHANNEL 14
uint8_t connectedClientCount = 0;
bool tryConnectExternal = false;
uint8_t channelToScanNext = MIN_SCAN_CHANNEL;
bool longerScanDelay = false;
bool scanActive = false;

void WiFiEvent(WiFiEvent_t event)
{
    //Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println(F("Disconnected from WiFi access point"));

            //We were connected externally so lets try again to connect
            tryConnectExternal = true;
            channelToScanNext = MIN_SCAN_CHANNEL;

            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            //We connected externally and got an ip.  Don't try again
            tryConnectExternal = false;
            channelToScanNext = MIN_SCAN_CHANNEL;

            Serial.print(F("Connected to external WiFi network ("));
            Serial.print(_externalSSID);
            Serial.print("): ");
            Serial.println(WiFi.localIP());
            break;        

            
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println(F("WiFi Client connected"));
            connectedClientCount++;
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println(F("WiFi Client disconnected"));
            connectedClientCount--;
            break;
        default: break;
    }
}

void _scanChannel(const uint8_t channel)
{
    //Serial.print("Scanning Channel: ");
    //Serial.println(channel);

    wifi_scan_config_t config;
    config.ssid = 0;
    config.bssid = 0;
    config.channel = channel;
    config.show_hidden = false;
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.min = 40;
    config.scan_time.active.max = 70;
    esp_wifi_scan_start(&config, false);

    scanActive = true;
}

void CANServer::Network::setup()
{
    Serial.println(F("Setting up Networking ..."));

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

    WiFi.setAutoReconnect(false);            //Since we manage reconnect stuff on our own we don't want to use the build in stuff

    WiFi.onEvent(WiFiEvent);

    //Lets see if we have wireless credentials to connect to
    _externalSSID = _networkingPrefs.getString("externalSSID", "");
    _externalPw = _networkingPrefs.getString("externalPw", "");
    if (_externalSSID.length() > 0)
    {
        //We have an external SSID configuration.  Setup as WIFI AP/STA mode
        Serial.print(F("Attempting to connect to external WiFi: "));
        Serial.println(_externalSSID);

        //Lets try and find the external SSID we want to connect to
        tryConnectExternal = true;
    }
    
    WiFi.mode(WIFI_AP_STA);

    //Now start up the Soft AP for the displays to connect to
    WiFi.softAP(displaySSID, displayPassword);
    IPAddress IP = WiFi.softAPIP();
    Serial.print(F("Soft AP IP address: "));
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

unsigned long previousMillisWiFiReconnect = 0;
void CANServer::Network::handle()
{
    if (tryConnectExternal)
    {
        //Sort out if we have any scan results
        //and figgure out if we need to do anything further
        bool foundExternalSSID = false;

        int scanReturnCode = WiFi.scanComplete();
        if (scanReturnCode >= 0)
        {
            scanActive = false;

            //Scan of the last channel is done.  Check to see if we see the SSID we want
            for (int i = 0; i < scanReturnCode; ++i) 
            {
                if (WiFi.SSID(i) == _externalSSID)
                {
                    foundExternalSSID = true;
                }
            }

            WiFi.scanDelete();

            if (foundExternalSSID)
            {
                //We can see the SSID we want to connec to.  Lets try and connect.
                Serial.println("Found SSID we want to connect to.  Lets try...");

                WiFi.begin(_externalSSID.c_str(), _externalPw.c_str());

                //Stop scanning
                tryConnectExternal = false;
            }
            else
            {
                //Nothing found yet.  Lets inc the channel number and scan again in a little bit
                channelToScanNext++;
                if (channelToScanNext > MAX_SCAN_CHANNEL)
                {
                    //When we get to the end of the channel list lets wrap around but with a longer delay
                    longerScanDelay = true;
                    channelToScanNext = MIN_SCAN_CHANNEL;
                }
            }
        }
        else
        {
            //No scan result yet.  We can just move on with life
        }
    }

    if (tryConnectExternal && !scanActive)
    {
        //We are still trying to connect and there isn't a scan running.
        //Lets try and scan the next channel if we have waited long enough.

        unsigned long currentMillis = millis();

        unsigned long scanDelay = 2000;
        if (longerScanDelay)
        {
            //Bump up to a minute between scans when we wrap around the channel numbers
            //This results in (scanDelay * 3)ms between each same channel scan, and a total scan time of (scanDelay * 14)ms for a full spectrum scan
            scanDelay *= 4;
        }
        if (currentMillis - previousMillisWiFiReconnect > scanDelay)
        {
            //Start up the next channel scan
            _scanChannel(channelToScanNext);

            previousMillisWiFiReconnect = currentMillis;
        }
    }
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