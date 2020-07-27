/*   CAN Server by Josh Wardell
 *   http://www.jwardell.com/canserver/
 *   To be used with microDisplay
 *
 *   July 7 2020
 *
 *   Board: Node32s
 *   (must press IO0 right button to start programming)
 */

#include "SerialPorts.h"
#include "Network.h"
#include "OTA.h"
#include "SDCard.h"
#include "CanBus.h"
#include "SPIFFileSystem.h"
#include "DisplayState.h"
#include "WebServer.h"
#include "PandaUDP.h"

#define LED1 1    //shared with serial tx - try not to use
#define LED2 2    //onboard blue LED
#define CFG2 4    //future

// Create Panda UDP server
PandaUDP panda;

void setup() {
    pinMode(LED2,OUTPUT);
    pinMode(CFG2,INPUT_PULLUP);
    
    CANServer::SerialPorts::setup();

    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println(__DATE__ " " __TIME__);
    
    //Bring up storage devices    
    CANServer::SPIFFileSystem::setup();
    CANServer::SDCard::setup();

    //Bring up network related components
    CANServer::Network::setup();
    CANServer::OTA::setup();

    // Begin Panda UDP server
    panda.begin();

    //Bring up CAN bus hardware
    CANServer::CanBus::setup();

    CANServer::DisplayState::loadAll();

    //Bring up Web server
    CANServer::WebServer::setup();

    //Spin up CAN bus and get it ready to process messages
    CANServer::CanBus::startup();
}


unsigned long previousMillis = 0;
void loop()
{
    //Deal with any pending OTA related work
    CANServer::Network::handle();
    CANServer::OTA::handle();
    CANServer::SerialPorts::handle();   

    CANServer::CanBus::handle();
}