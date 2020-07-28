#include "CanBus.h"

#include <Arduino.h>
#include <esp32_can.h>

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

namespace CANServer
{
    namespace CanBus
    {

        const uint16_t _SwapEndian16 (const uint8_t *pos)
        {
            uint16_t return_val;

            return_val = pos[0];
            return_val = (return_val << 8) | pos[1];

            return(return_val);
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


        void setup()
        {
            Serial.println("Setting up Can-Bus...");

            CAN0.begin(BITRATE);

#ifdef CAN1_ENABLED      
            CAN1.begin(BITRATE);
#endif

#ifdef UDPCAN_ENABLED
            canudp.begin();
#endif

            Serial.println("Done");
        }

        void startup()
        {
            //  CAN0.watchFor(0x100, 0xF00); //setup a special filter
            CAN0.watchFor(); //then let everything else through anyway
            //  CAN0.setCallback(0, gotHundred); //callback on that first special filter
        }

        void printFrame(CAN_FRAME *frame)
        {
            // Print message
            Serial.print("ID: ");
            Serial.print(frame->id,HEX);
            Serial.print(", Ext: ");
            if(frame->extended) {
                Serial.print("Y");
            } else {
                Serial.print("N");
            }
            Serial.print(", Len: ");
            Serial.print(frame->length,DEC);
            Serial.print(": ");
            for(int i = 0;i < frame->length; i++) 
            {
                Serial.print(frame->data.uint8[i],HEX);
                Serial.print(" ");
            }
            Serial.println();
        }

        
        void _processMessage(CAN_FRAME *frame, const uint8_t busId);

        void handle()
        {
            CAN_FRAME message;
            if (CAN0.read(message)) 
            {
                digitalWrite(2, !digitalRead(2)); //flash LED2 to show data Rx
                _processMessage(&message, 0);
            }
#ifdef CAN1_ENABLED            
            if (CAN1.read(message))
            {
                _processMessage(&message, 1);
            }
#endif
#ifdef UDPCAN_ENABLED
            if (canudp.read(message))
            {
                _processMessage(&message, 99);
            }
#endif
        }

        void _processMessage(CAN_FRAME *frame, const uint8_t busId)
        {
            //printFrame(frame);
            
            //Pass this message off to a panda client if one is registered
            panda.handleMessage(*frame);

            //Let the logging code deal with this frame as well
            CANServer::Logging::instance()->handleMessage(frame, busId);

            //Now process this frame and sort out any data we care about
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
    }
}