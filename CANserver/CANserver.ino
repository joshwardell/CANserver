/*   CAN Server by Josh Wardell
 *   http://www.jwardell.com/canserver/
 *   To be used with microDisplay
 *
 *   July 7 2020
 *
 *   Board: Node32s
 *   (must press IO0 right button to start programming)
 */
#include "generalCANSignalAnalysis.h" //https://github.com/iChris93/ArduinoLibraryForCANSignalAnalysis

#include "SerialPorts.h"
#include "AccessPoint.h"
#include "OTA.h"
#include "SDCard.h"
#include "CanBus.h"
#include "SPIFFileSystem.h"
#include "WebServer.h"

#define SETTINGSFILE "/settings.json"

generalCANSignalAnalysis analyzeMessage; //initialize library

#define LED1 1    //shared with serial tx - try not to use
#define LED2 2    //onboard blue LED
#define CFG1 15   //jumper to toggle second CANserver
#define CFG2 4    //future


#define littleEndian true
#define bigEndian false

// access point network credentials - don't change these
const char* ssid = "CANserver";
const char* password = "JWcanServer2020";

void setup(){
  
    //pinMode(LED1,OUTPUT); // LED1 shares TXpin with serial
    pinMode(LED2,OUTPUT);
    pinMode(CFG1,INPUT_PULLUP);
    pinMode(CFG2,INPUT_PULLUP);
    
    if (digitalRead(CFG1) == 0) { //If jumpered, server 2
        ssid = "CANserver2";
    }
    
    CANServer::SerialPorts::setup();
    
    //Bring up storage devices    
    CANServer::SPIFFileSystem::setup();
    CANServer::SDCard::setup();

    //Bring up network related components
    CANServer::AccessPoint::setup(ssid, password);
    CANServer::OTA::setup(ssid, password);

    //Bring up CAN bus hardware
    CANServer::CanBus::setup();

    //Bring up Web server
    CANServer::WebServer::setup();

    //Spin up CAN bus and get it ready to process messages
    CANServer::CanBus::startup();
}

void loop(){
    //Deal with any pending OTA related work
    CANServer::OTA::handle();
    
    CANServer::SerialPorts::handle();
    
    CANServer::CanBus::handle();
}
