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

    //Make some changes to the DHCP server configuration so we don't serve a DNS server or Router to clients
    //This helps phones not loose their wider network when connected to the CANServer's wifi
    tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
    uint8_t opt_val = 0; // don't supply a dns server via dhcps
    tcpip_adapter_dhcps_option(TCPIP_ADAPTER_OP_SET, TCPIP_ADAPTER_DOMAIN_NAME_SERVER, &opt_val, 1);  // don't supply a dns server via dhcps
	tcpip_adapter_dhcps_option(TCPIP_ADAPTER_OP_SET, TCPIP_ADAPTER_ROUTER_SOLICITATION_ADDRESS, &opt_val, 1);  // don't supply a gateway (router) via dhcps option 3

    tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);

    Serial.println("Done");
}