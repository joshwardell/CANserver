#include "CanBus.h"

#include <Arduino.h>
#include "esp32_can/esp32_can.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "generalCANSignalAnalysis.h" //https://github.com/iChris93/ArduinoLibraryForCANSignalAnalysis
#include "pandaUDP.h"
#include "CANUDP.h"
#include "Logging.h"



generalCANSignalAnalysis analyzeMessage; //initialize library

#define BITRATE 500000  //CAN bitrate, Tesla=500000
#define littleEndian true
#define bigEndian false

//Make sure the compiler knows this exists in our memory space
extern PandaUDP panda;

#ifdef UDPCAN_ENABLED
extern CANServer::CANUDP canudp;
#endif


CANServer::CanBus::AnalysisItem::AnalysisItem()
{
    frameId = 0;
    startBit = 0;
    bitLength = 0;
    factor = 0;
    signalOffset = 0;
    isSigned = false;
    byteOrder = false;

    builtIn = false;

    lastValue = 0;
}


CANServer::CanBus* CANServer::CanBus::_instance = NULL;

CANServer::CanBus* CANServer::CanBus::instance()
{
      if (_instance == NULL)
    {
        _instance = new CANServer::CanBus();
    }

    return _instance;
}
CANServer::CanBus::~CanBus()
{

}
CANServer::CanBus::CanBus()
{
    memset(&_workingCANFrame, 0, sizeof(CAN_FRAME));

    _dynamicAnalysisPaused = false;
}

void CANServer::CanBus::setup()
{
    log_i("Setting up Can-Bus...");

    this->_loadDynamicAnalysisConfiguration();

    //Start up the onboard can transcever
    CAN0.begin(BITRATE);

#ifdef CAN1_ENABLED      
    //Start up the secondary SPI can transcever
    CAN1.begin(BITRATE);
#endif

#ifdef UDPCAN_ENABLED
    //Start up the UDP can udp packet receiver
    canudp.begin();
#endif

    log_i("Done");
}

void CANServer::CanBus::startup()
{
    CAN0.watchFor();
#ifdef CAN1_ENABLED            
    CAN1.watchFor();
#endif
}

void CANServer::CanBus::handle()
{
    if (CAN0.read(_workingCANFrame)) 
    {
        digitalWrite(2, !digitalRead(2)); //flash LED2 to show data Rx
        this->_processFrame(&_workingCANFrame, 0);
    }
#ifdef CAN1_ENABLED            
    if (CAN1.read(_workingCANFrame))
    {
        this->_processFrame(&_workingCANFrame, 1);
    }
#endif
#ifdef UDPCAN_ENABLED
    if (canudp.read(_workingCANFrame))
    {
        this->_processFrame(&_workingCANFrame, 99);
    }
#endif

    //TODO: Run any of our scripts
}

void CANServer::CanBus::_processFrame(CAN_FRAME *frame, const uint8_t busId)
{
    //Pass this message off to a panda client if one is registered
    panda.handleMessage(*frame);

    //Let the logging code deal with this frame as well
    CANServer::Logging::instance()->handleMessage(frame, busId);

    this->_processStaticAnalysis(frame);
    this->_processDynamicAnalysis(frame);
}

const uint32_t _SwapEndian32 (const uint8_t *pos)
{
    uint32_t return_val;

    return_val = pos[0];
    return_val = (return_val << 8) | pos[1];
    return_val = (return_val << 8) | pos[2];
    return_val = (return_val << 8) | pos[3];

    return(return_val);
}

void CANServer::CanBus::_processStaticAnalysis(CAN_FRAME *frame)
{
    switch(frame->id)
    {                        
        case 0x528:   //ID528UnixTime
        {
            //This is the UNIX time message.  Verify that its the right size
            if (frame->length == 4)
            {
                //Convert this to a time_t
                uint32_t currentTimestamp =_SwapEndian32(frame->data.bytes);
                
                //Set the system time from the message we just received over the canbus
                struct timeval tv = {(time_t)currentTimestamp, 0};
                settimeofday(&tv, NULL);
            }
            break;
        }
    }
}

void CANServer::CanBus::pauseDynamicAnalysis()
{
    _dynamicAnalysisPaused = true;
}
void CANServer::CanBus::resumeDynamicAnalysis()
{
    _dynamicAnalysisPaused = false;
}

void CANServer::CanBus::saveDynamicAnalysisFile(const char* itemName)
{
    AnalysisItemMap::const_iterator it = _analysisItems.find(itemName);
    if (it != _analysisItems.end()) 
    {
        AnalysisItem *analysisItem = it->second;
        String newConfigFile = String(F("/a/")) + String(it->first.c_str());

        File file = SPIFFS.open(newConfigFile, FILE_WRITE);
        if (!file) {
            Serial.println(F("Failed to create file"));
            return;
        }

        StaticJsonDocument<256> doc;

        // Set the values in the document
        doc["n"] = it->first;
        doc["fid"] =  analysisItem->frameId;
        doc["sb"] =  analysisItem->startBit;
        doc["bl"] =  analysisItem->bitLength;
        doc["f"] =  analysisItem->factor;
        doc["so"] =  analysisItem->signalOffset;
        doc["s"] =  analysisItem->isSigned;
        doc["bo"] =  analysisItem->byteOrder;

        doc["bi"] =  analysisItem->builtIn;

        // Serialize JSON to file
        if (serializeJson(doc, file) == 0) 
        {
            Serial.println(F("Failed to write to file"));
        }

        // Close the file
        file.close();
    }
}
void CANServer::CanBus::deleteDynamicAnalysisFile(const char* itemName)
{
    String fileName = String(F("/a/")) + itemName;
    SPIFFS.remove(fileName);
}

void CANServer::CanBus::resolveLookups()
{
    _displayOnAnalysisItem = NULL;
    _distanceUnitMilesAnalysisItem = NULL;
    _quickFrameIdLookup_analysisItems.clear();

    //Sort out our quick look strcuture for processing
    for (AnalysisItemMap::iterator it = _analysisItems.begin(); it != _analysisItems.end(); it++)
    {
        //Store some pointers for some quick lookups that we need (a bunch of our predefined things that we use for multiple things)
        if (it->first == "DisplayOn")
        {
            _displayOnAnalysisItem = it->second;
        } 
        else if (it->first == "DistanceUnitMiles")
        {
            _distanceUnitMilesAnalysisItem = it->second;
        }
        
        AnalysisItemFrameLookupMap::iterator lookupIt = _quickFrameIdLookup_analysisItems.find(it->second->frameId);
        if (lookupIt == _quickFrameIdLookup_analysisItems.end())
        {
            std::pair<AnalysisItemFrameLookupMap::iterator, bool> returnPair = _quickFrameIdLookup_analysisItems.insert(AnalysisItemFrameLookupPair(it->second->frameId, std::list<AnalysisItem*>()));
            lookupIt = returnPair.first;
        }

        lookupIt->second.push_back(it->second);
    }
}

void CANServer::CanBus::_loadDynamicAnalysisConfiguration()
{
    //Try and find all the diffrent analysis files stored on disk
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) 
    {
        String fileName = file.name();
        if (fileName.startsWith("/a/"))
        {
            Serial.print(F("Loading Dynamic Analysis Config: "));
            Serial.println(fileName);

            //We care about this file.  Lets open it and try and load
            StaticJsonDocument<256> doc;

            DeserializationError error = deserializeJson(doc, file);
            if (error)
                Serial.println(F("Failed to read file, using default configuration"));


            //Get the contents of the file and setup our objects
            AnalysisItem *newAnalysisItem = new AnalysisItem();
            newAnalysisItem->frameId = doc["fid"];
            newAnalysisItem->startBit = doc["sb"];
            newAnalysisItem->bitLength = doc["bl"];
            newAnalysisItem->factor = doc["f"];
            newAnalysisItem->signalOffset = doc["so"];
            newAnalysisItem->isSigned = doc["s"];
            newAnalysisItem->byteOrder = doc["bo"];

            newAnalysisItem->builtIn = doc["bi"] || false;
            
            const char* name = doc["n"];
            _analysisItems.insert(AnalysisItemPair(name, newAnalysisItem));
        }

        file = root.openNextFile();
    }

    this->resolveLookups();
}


void CANServer::CanBus::_processDynamicAnalysis(CAN_FRAME *frame)
{
    if (!_dynamicAnalysisPaused)
    {
        AnalysisItemFrameLookupMap::iterator lookupIt = _quickFrameIdLookup_analysisItems.find(frame->id);
        if (lookupIt != _quickFrameIdLookup_analysisItems.end())
        {
            for (std::list<AnalysisItem*>::iterator listIt = lookupIt->second.begin(); listIt != lookupIt->second.end(); listIt++)
            {
                AnalysisItem *analysisItem = *listIt;

                if (analysisItem->frameId == frame->id)
                {
                    analysisItem->lastValue = analyzeMessage.getSignal(frame->data.uint64, analysisItem->startBit, analysisItem->bitLength, analysisItem->factor, analysisItem->signalOffset, analysisItem->isSigned, analysisItem->byteOrder);
                }        
            } 
        }
    }
}