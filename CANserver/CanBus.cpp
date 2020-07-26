#include "CanBus.h"

#include <Arduino.h>
#include <esp32_can.h>
#include <SD.h>
#include <SPIFFS.h>
#include <Preferences.h>


#include "VehicleState.h"
#include "generalCANSignalAnalysis.h" //https://github.com/iChris93/ArduinoLibraryForCANSignalAnalysis
#include "pandaUDP.h"
#include "SDCard.h"

generalCANSignalAnalysis analyzeMessage; //initialize library

#define BITRATE 500000  //CAN bitrate, Tesla=500000
#define littleEndian true
#define bigEndian false

//Make sure the compiler knows this exists in our memory space
extern PandaUDP panda;


namespace CANServer
{
    namespace CanBus
    {
        Preferences _prefs;
        SDFile _rawCanLogFile;

        bool logRawCan = false;

        void loadSettings()
        {
            logRawCan = _prefs.getBool("lograw", false);
        }

        void saveSettings()
        {
             _prefs.putBool("lograw", logRawCan);
        }

        void setup()
        {
            Serial.println("Setting up Can-Bus...");

            CAN0.begin(BITRATE);

            _prefs.begin("CanBus");
            
            loadSettings();
            if (logRawCan) 
            {
                openRawLog();
            }

            Serial.println("Done");
        }

        void openRawLog()
        {
            if (CANServer::SDCard::available())
            {
                if (!_rawCanLogFile)
                {
                    _rawCanLogFile = SD.open("/" RAWCANLOGNAME, FILE_APPEND);

                    if (_rawCanLogFile)
                    {
                        Serial.println("Logging raw messages to SD");
                    }
                    else
                    {
                        Serial.println("Unable to open raw log file");
                    }
                }
            }
            else
            {
                _rawCanLogFile.close();
            }
        }

        void closeRawLog()
        {
            if (CANServer::SDCard::available() && _rawCanLogFile)
            {
                _rawCanLogFile.close();
            }
        }

        void startup()
        {
            //  CAN0.watchFor(0x100, 0xF00); //setup a special filter
            CAN0.watchFor(); //then let everything else through anyway
            //  CAN0.setCallback(0, gotHundred); //callback on that first special filter
        }

void printFrame(CAN_FRAME &frame)
{
	// Print message
	Serial.print("ID: ");
	Serial.println(frame.id,HEX);
	Serial.print("Ext: ");
	if(frame.extended) {
		Serial.println("Y");
	} else {
		Serial.println("N");
	}
	Serial.print("Len: ");
	Serial.println(frame.length,DEC);
	for(int i = 0;i < frame.length; i++) {
		Serial.print(frame.data.uint8[i],HEX);
		Serial.print(" ");
	}
	Serial.println();
}

        void handle()
        {
            CAN_FRAME message;
            if (CAN0.read(message)) {
                //printFrame(message);
                // Send to UDP server if a client is connected.
                panda.handleMessage(message);

                if (logRawCan && _rawCanLogFile)
                {
                    //Log this message to the log file

                    //(1551774790.942758) can1 7A8 [8] F4 DC D1 83 0E 02 00 00
                    _rawCanLogFile.print("(0000000.0000) can1 ");
                    _rawCanLogFile.print(message.id, HEX);
                    _rawCanLogFile.print(" [");
                    _rawCanLogFile.print(message.length, DEC);
                    _rawCanLogFile.print("] ");
                    for (int i = 0; i < message.length; i++) 
                    {
                        if (i > 0) { _rawCanLogFile.print(" "); }
                        _rawCanLogFile.print(message.data.byte[i], HEX);
                    
                    }
                    _rawCanLogFile.println();
                    _rawCanLogFile.flush();   
                }

                /*    Serial.print(message.id, HEX);  ///debug display RX message
                if (message.extended) Serial.print(" X ");
                else Serial.print(" S ");
                Serial.print(message.length, DEC);
                for (int i = 0; i < message.length; i++) {
                Serial.print(message.data.byte[i], HEX);
                Serial.print(" ");
                }
                Serial.println();
                */
                digitalWrite(2, !digitalRead(2)); //flash LED2 to show data Rx

                CANServer::VehicleState* vehicleState = CANServer::VehicleState::instance();

                switch (message.id)
                {
                    case 0x00C:
                        if (message.length == 8) {
                            vehicleState->DisplayOn = analyzeMessage.getSignal(message.data.uint64, 5, 1, 1, 0, false, littleEndian);  //SG_ UI_displayOn : 5|1@1+ (1,0) [0|1] ""
                        }
                        break;
                        
                    case 0x132:
                        if (message.length == 8) {
                            int tempvolts;
                            tempvolts = analyzeMessage.getSignal(message.data.uint64, 0, 16, 0.01, 0, false, littleEndian);
                            if ((tempvolts > 290) && (tempvolts < 420)) { //avoid some bad messages
                                vehicleState->BattVolts = tempvolts;
                                vehicleState->BattAmps = analyzeMessage.getSignal(message.data.uint64, 16, 16, -0.1, 0, true, littleEndian); //signed 15, mask off sign

                                //Power is a synthisized value.  Calculate it
                                vehicleState->BattPower = vehicleState->BattVolts * vehicleState->BattAmps / 100;
                            }
                        }
                        break;
                        
                    case 0x1D8:
                        if (message.length == 8) {
                            int temptorque;
                            temptorque = analyzeMessage.getSignal(message.data.uint64, 24, 13, 0.25, 0, true, littleEndian);  //signed13, mask off sign
                            if ((temptorque < 5000) && (temptorque > -2000)) {  //reduce errors
                                vehicleState->RearTorque = temptorque;
                            }
                        }
                        break;
                        
                    case 0x312:
                        if (message.length == 8) {
                            vehicleState->MinBattTemp = analyzeMessage.getSignal(message.data.uint64, 44, 9, 0.25, -25, false, littleEndian) * 1.8 + 32;
                        }
                        break;
                        /*
                        case 0x224:
                        DCDCoutput224 = ((message.data.byte[3] << 8) + message.data.byte[2]) & 0x07FF;
                        break;
                        */
                    case 0x241:
                        vehicleState->BattCoolantRate = analyzeMessage.getSignal(message.data.uint64, 0, 9, 0.1, 0, false, littleEndian);  //ID 241 SB 0 u9 scale .01 LPM
                        vehicleState->PTCoolantRate = analyzeMessage.getSignal(message.data.uint64, 22, 9, 0.1, 0, false, littleEndian);    //ID 241 SB 22 u9 scale .01 LPM
                        break;
                        
                        /*      case 0x261:
                        v12v261 = (((message.data.byte[1] & 0x0F) << 12) | (message.data.byte[0] << 4)) >> 4;  //ID 261 SB0 u12 scale 0.005444 V
                        break;
                        */
                    case 0x252:
                        vehicleState->MaxRegen =  analyzeMessage.getSignal(message.data.uint64, 0, 16, 0.01, 0, false, littleEndian);
                        vehicleState->MaxDisChg = analyzeMessage.getSignal(message.data.uint64, 16, 16, 0.01, 0, false, littleEndian);
                        break;
                        
                    case 0x257: //VehSpeed = 0;     //ID 257 SB 12 u12 scale .08 offset -40 KPH
                        if (message.length == 8) {
                            ///SpeedUnit = analyzeMessage.getSignal(message.data.uint64, 32, 1, 1, 0, false, littleEndian); //strange this doesn't change with UI setting! Location?
                            vehicleState->VehSpeed = analyzeMessage.getSignal(message.data.uint64, 12, 12, 0.08, -40, false, littleEndian);
                        }
                        break;
                        
                    case 0x293:    
                        if (message.length == 8) {
                            vehicleState->SpeedUnit = analyzeMessage.getSignal(message.data.uint64, 13, 1, 1, 0, false, littleEndian);      //UI distance setting to toggle speed display units
                        }
                        break;
                                        
                    case 0x399: //Blind spots
                        if (message.length == 8) {
                            vehicleState->BSR = analyzeMessage.getSignal(message.data.uint64, 6, 2, 1, 0, false, littleEndian);
                            vehicleState->BSL = analyzeMessage.getSignal(message.data.uint64, 4, 2, 1, 0, false, littleEndian);
                            if (vehicleState->BSR > 2) {  // 3 is active but no warning
                                vehicleState->BSR = 0;
                            }
                            if (vehicleState->BSL > 2) {
                                vehicleState->BSL = 0;
                            }
                        }
                        break;
                        /*      ///////////
                        case 0x292:
                        SOCUI = (((message.data.byte[1] & 0x03) << 8) + message.data.byte[0]);
                        SOCAVE = (((message.data.byte[4] << 8) + message.data.byte[3]) & 0xFFC0) >> 6;  //ID 292 SB30 u10 scale 0.1 %
                        break;
                        
                        case 0x321:
                        BattCoolantTemp = ((message.data.byte[1] & 0x03) << 8) + message.data.byte[0];  //ID 321 SB0 u10 scale 0.125 offset -40 C
                        PTCoolantTemp = (((message.data.byte[2] << 8) + message.data.byte[1]) & 0x0FFC) >> 2;    //ID 321 SB10 u10 scale 0.125 offset -40 C
                        break;
                        
                        case 0x352:
                        BattFullKWh = ((message.data.byte[1] & 0x03) * 256) + message.data.byte[0];
                        BattRemainKWh = ((message.data.byte[2] & 0x0F) * 64) + (message.data.byte[1] >> 2);
                        //Serial.print(BattRemainKWh);
                        //Serial.print(" ");
                        break;
                        
                        case 0x315:
                        InvHStemp376 = message.data.byte[1];
                        break;
                        
                             case 0x712:
                        if (message.data.byte[0] = 0); //index 0
                        {
                        CellTemp1 = message.data.byte[1];
                        CellTemp2 = message.data.byte[2];
                        }
                        break;
                        */
                        
                    default:
                        break;
                } //end can msg id case
                
            } //endif can rec msg
        }
    }
}