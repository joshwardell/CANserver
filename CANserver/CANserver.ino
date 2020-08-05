/*   CAN Server by Josh Wardell and Chris Whiteford
 *   http://www.jwardell.com/canserver/
 *   To be used with microDisplay
 *
 *   July 27 2020
 *
 *   Board: Node32s
 *   (must press IO0 right button to start programming)
 */
#include "BuildRev.h"
#include "SerialPorts.h"
#include "Network.h"
#include "SDCard.h"
#include "CanBus.h"
#include "SPIFFileSystem.h"
#include "Displays.h"
#include "WebServer.h"
#include "PandaUDP.h"
#include "CANUDP.h"
#include "Logging.h"
#include "Average.h"

#define LED1 1    //shared with serial tx - try not to use
#define LED2 2    //onboard blue LED
#define CFG2 4    //future

// Create Panda UDP server
PandaUDP panda;

#ifdef UDPCAN_ENABLED
CANServer::CANUDP canudp;
#endif

void setup() {
    pinMode(LED2,OUTPUT);
    pinMode(CFG2,INPUT_PULLUP);
    
    CANServer::SerialPorts::setup();

    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println(__DATE__ " " __TIME__ " - " BUILD_REV);
    
    //Bring up storage devices    
    CANServer::SPIFFileSystem::setup();
    CANServer::SDCard::setup();

    //Bring up network related components
    CANServer::Network::setup();

    // Begin Panda UDP server
    panda.begin();

    CANServer::Logging::instance()->setup();

    //Bring up CAN bus hardware
    CANServer::CanBus::instance()->setup();

    CANServer::Displays::instance()->setup();

    //Bring up Web server
    CANServer::WebServer::setup();

    //Spin up CAN bus and get it ready to process messages
    CANServer::CanBus::instance()->startup();
}

unsigned long previousMillisMemoryOutput = 0;
unsigned long previousMillisLEDBlink = 0;

Average<unsigned long> _loopTime(20);
Average<uint32_t> _memoryUsage(60);
uint8_t memorySampleCounter = 0;

extern bool RebootAfterUpdate;
extern bool PauseAllProcessing;

void loop()
{
    unsigned long loopTimeStart = millis();
    //Deal with any pending OTA related work
    CANServer::Network::handle();

    if (!PauseAllProcessing)
    {
        CANServer::SerialPorts::handle();   

        CANServer::CanBus::instance()->handle();
    }
    
    _loopTime.push(millis() - loopTimeStart);

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisMemoryOutput >= 100) 
    {
        previousMillisMemoryOutput = currentMillis;
        _memoryUsage.push(ESP.getFreeHeap());

        if (memorySampleCounter++ > 10)
        {
            log_v("RAM Usage: mean: %0.2f, max: %d, min: %d, stdev: %0.2f", _memoryUsage.mean(), _memoryUsage.maximum(), _memoryUsage.minimum(), _memoryUsage.stddev());
            //log_v("Lopp Time: mean: %0.2f, max: %d, min: %d, stdev: %0.2f", _loopTime.mean(), _loopTime.maximum(), _loopTime.minimum(), _loopTime.stddev());
            memorySampleCounter = 0;
        }
    }

    if (currentMillis - previousMillisLEDBlink >= 500) 
    {
        //Blink a heartbeat LED
        previousMillisLEDBlink = currentMillis;
        digitalWrite(LED2, !digitalRead(LED2));
    }

    if (RebootAfterUpdate)
    {
        delay(1000);
        ESP.restart();
    }
}