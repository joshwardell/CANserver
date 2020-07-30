#include "CanBus.h"

#include <Arduino.h>
#include <esp32_can.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "VehicleState.h"
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
    _frameId = 0;
    _startBit = 0;
    _bitLength = 0;
    _factor = 0;
    _signalOffset = 0;
    _isSigned = false;
    _byteOrder = false;

    _lastValue = 0;
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
    
    /*AnalysisItem* test = new AnalysisItem();
    test->_frameId = 0x132;
    test->_startBit = 0;
    test->_bitLength = 16;
    test->_factor = 0.01;
    test->_signalOffset = 0;
    test->_isSigned = false;
    test->_byteOrder = littleEndian;

    _analysisItems.insert(AnalysisItemPair("TestVolts", test));*/
}

void CANServer::CanBus::setup()
{
    Serial.println(F("Setting up Can-Bus..."));

    CAN0.begin(BITRATE);

#ifdef CAN1_ENABLED      
    CAN1.begin(BITRATE);
#endif

#ifdef UDPCAN_ENABLED
    canudp.begin();
#endif

    this->_loadDynamicAnalysisConfiguration();

    Serial.println(F("Done"));
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
    CANServer::VehicleState* vehicleState = CANServer::VehicleState::instance();
    switch(frame->id)
    {
        case 0x00C:     //ID00CUI_status
        {
            if (frame->length == 8) {
                vehicleState->DisplayOn = analyzeMessage.getSignal(frame->data.uint64, 5, 1, 1, 0, false, littleEndian);  //SG_ UI_displayOn : 5|1@1+ (1,0) [0|1] ""
            }
            break;
        }

        case 0x132:     //ID132HVBattAmpVolt
        {
            if (frame->length == 8) {
                int tempvolts;
                tempvolts = analyzeMessage.getSignal(frame->data.uint64, 0, 16, 0.01, 0, false, littleEndian);
                if ((tempvolts > 290) && (tempvolts < 420)) { //avoid some bad messages
                    vehicleState->BattVolts = tempvolts;
                    vehicleState->BattAmps = analyzeMessage.getSignal(frame->data.uint64, 16, 16, -0.1, 0, true, littleEndian); //signed 15, mask off sign

                    //Power is a synthisized value.  Calculate it
                    vehicleState->BattPower = vehicleState->BattVolts * vehicleState->BattAmps / 100;
                }
            }
            break;
        }

        case 0x1D8:
        {
            if (frame->length == 8) {
                int temptorque;
                temptorque = analyzeMessage.getSignal(frame->data.uint64, 24, 13, 0.25, 0, true, littleEndian);  //signed13, mask off sign
                if ((temptorque < 5000) && (temptorque > -2000)) {  //reduce errors
                    vehicleState->RearTorque = temptorque;
                }
            }
            break;
        }
                
        case 0x312:
        {
            if (frame->length == 8) {
                vehicleState->MinBattTemp = analyzeMessage.getSignal(frame->data.uint64, 44, 9, 0.25, -25, false, littleEndian) * 1.8 + 32;
            }
            break;
        }
                
        case 0x241:
        {
            vehicleState->BattCoolantRate = analyzeMessage.getSignal(frame->data.uint64, 0, 9, 0.1, 0, false, littleEndian);  //ID 241 SB 0 u9 scale .01 LPM
            vehicleState->PTCoolantRate = analyzeMessage.getSignal(frame->data.uint64, 22, 9, 0.1, 0, false, littleEndian);    //ID 241 SB 22 u9 scale .01 LPM
            break;
        }
                
        case 0x252:
        {
            vehicleState->MaxRegen =  analyzeMessage.getSignal(frame->data.uint64, 0, 16, 0.01, 0, false, littleEndian);
            vehicleState->MaxDisChg = analyzeMessage.getSignal(frame->data.uint64, 16, 16, 0.01, 0, false, littleEndian);
            break;
        }
                
        case 0x257: //VehSpeed = 0;     //ID 257 SB 12 u12 scale .08 offset -40 KPH
        {
            if (frame->length == 8) {
                vehicleState->VehSpeed = analyzeMessage.getSignal(frame->data.uint64, 12, 12, 0.08, -40, false, littleEndian);
            }
            break;
        }
                
        case 0x293:    
        {
            if (frame->length == 8) {
                vehicleState->SpeedUnit = analyzeMessage.getSignal(frame->data.uint64, 13, 1, 1, 0, false, littleEndian);      //UI distance setting to toggle speed display units
            }
            break;
        }
                                
        case 0x399: //Blind spots
        {
            if (frame->length == 8) {
                vehicleState->BSR = analyzeMessage.getSignal(frame->data.uint64, 6, 2, 1, 0, false, littleEndian);
                vehicleState->BSL = analyzeMessage.getSignal(frame->data.uint64, 4, 2, 1, 0, false, littleEndian);
                if (vehicleState->BSR > 2) {  // 3 is active but no warning
                    vehicleState->BSR = 0;
                }
                if (vehicleState->BSL > 2) {
                    vehicleState->BSL = 0;
                }
            }
            break;
        }

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

void CANServer::CanBus::saveDynamicAnalysisConfiguration()
{
    //Delete all the existing config files
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) 
    {
        String fileName = file.name();
        if (fileName.startsWith(F("/analysis/")))
        {
            SPIFFS.remove(fileName);
        }

        file = root.openNextFile();
    }

    for (AnalysisItemMap::const_iterator it = _analysisItems.begin(); it != _analysisItems.end(); it++)
    {
        AnalysisItem *analysisItem = it->second;
        String newConfigFile = String(F("/analysis/")) + String(it->first.c_str()) + String(F(".json"));

        File file = SPIFFS.open(newConfigFile, FILE_WRITE);
        if (!file) {
            Serial.println(F("Failed to create file"));
            return;
        }

        StaticJsonDocument<256> doc;

        // Set the values in the document
        doc["n"] = it->first;
        doc["fid"] =  analysisItem->_frameId;
        doc["sb"] =  analysisItem->_startBit;
        doc["bl"] =  analysisItem->_bitLength;
        doc["f"] =  analysisItem->_factor;
        doc["so"] =  analysisItem->_signalOffset;
        doc["s"] =  analysisItem->_isSigned;
        doc["bo"] =  analysisItem->_byteOrder;

        // Serialize JSON to file
        if (serializeJson(doc, file) == 0) 
        {
            Serial.println(F("Failed to write to file"));
        }

        // Close the file
        file.close();
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
        if (fileName.startsWith("/analysis/"))
        {
            Serial.println(F("Loading Dynamic Analysis Config: "));
            Serial.println(fileName);

            //We care about this file.  Lets open it and try and load
            StaticJsonDocument<256> doc;

            DeserializationError error = deserializeJson(doc, file);
            if (error)
                Serial.println(F("Failed to read file, using default configuration"));


            //Get the contents of the file and setup our objects
            AnalysisItem *newAnalysisItem = new AnalysisItem();
            newAnalysisItem->_frameId = doc["fid"];
            newAnalysisItem->_startBit = doc["sb"];
            newAnalysisItem->_bitLength = doc["bl"];
            newAnalysisItem->_factor = doc["f"];
            newAnalysisItem->_signalOffset = doc["so"];
            newAnalysisItem->_isSigned = doc["s"];
            newAnalysisItem->_byteOrder = doc["bo"];
            
            const char* name = doc["n"];
            _analysisItems.insert(AnalysisItemPair(name, newAnalysisItem));

        }

        file = root.openNextFile();
    }

}

void CANServer::CanBus::_processDynamicAnalysis(CAN_FRAME *frame)
{
    if (!_dynamicAnalysisPaused)
    {
        for (AnalysisItemMap::const_iterator it = _analysisItems.begin(); it != _analysisItems.end(); it++)
        {
            AnalysisItem *analysisItem = it->second;

            if (analysisItem->_frameId == frame->id)
            {
                analysisItem->_lastValue = analyzeMessage.getSignal(frame->data.uint64, analysisItem->_startBit, analysisItem->_bitLength, analysisItem->_factor, analysisItem->_signalOffset, analysisItem->_isSigned, analysisItem->_byteOrder);
            }        
        }
    }
}