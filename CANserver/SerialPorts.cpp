#include "SerialPorts.h"

#include <Arduino.h>

namespace CANServer
{
    namespace SerialPorts
    {
        void setup()
        {
            Serial.begin(57600);
            Serial1.begin(57600, SERIAL_8N1, 32, 10);
        }

        void handle()
        {
            if (Serial) {
                //long currentMillis = millis();

                /*if (currentMillis - previouscycle >= 500) { //Every 500ms
                    previouscycle = currentMillis;
                    digitalWrite(LED2, !digitalRead(LED2)); //flash LED for awake
                    //digitalWrite(LED1, !digitalRead(LED1)); //debug LED1
                    Serial.print("BattAmps(a):"); //print data to serial for debug
                    Serial.print(BattAmps);
                    Serial.print(" BattVolts(b):");
                    Serial.print(BattVolts);
                    Serial.print(" BattPower(c):");
                    Serial.print(BattPower);
                    Serial.print(" RearTorque(d):");
                    Serial.print(RearTorque);
                    Serial.print(" MinBattTemp(e):");
                    Serial.print(MinBattTemp);
                    Serial.print(" MaxRegen(f):");
                    Serial.print(MaxRegen);
                    Serial.print(" MaxDisChg(g):");
                    Serial.print(MaxDisChg);
                    Serial.print(" VehSpeed(h):");
                    Serial.print(VehSpeed);
                    Serial.print(" BSR(i):");
                    Serial.print(BSR);
                    Serial.print(" BSL(j):");
                    Serial.print(BSL);
                    Serial.print(digitalRead(CFG1));
                    Serial.print(digitalRead(CFG2));
                    Serial.println();
                }*/
            }//if serial

#ifdef dorealstuff    
            if (Serial.available() > 0) {
                int inChar = Serial.read();   //read sample data for debug
                switch (inChar) {
                    case 'a':
                        BattAmps = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'b':
                        BattVolts = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'c':
                        BattPower = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'd':
                        RearTorque = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'e':
                        MinBattTemp = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'f':
                        MaxRegen = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'g':
                        MaxDisChg = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'h':
                        VehSpeed = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'i':
                        BSR = inString.toInt();
                        inString = "";
                        break;
                        
                    case 'j':
                        BSL = inString.toInt();
                        inString = "";
                        break;

                    case 'm':
                        DisplayOn = inString.toInt();
                        inString = "";
                        break;
                        
                    case '\n':
                        serialinput = inString.toInt(); //save off input
                        inString = "";
                        break;
                        
                    default:
                        inString += (char)inChar;
                        break;
                }
            } //serial available

#endif
        }
    }
}
