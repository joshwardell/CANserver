//
//  CANSignalAnalysis.h
//  To be used with CANserver created by Josh Wardell
//
//  Created by Chris Allemang on Jun 9 2020.
//

#ifndef CANSignalAnalysis_h
#define CANSignalAnalysis_h

#include "Arduino.h"

class CANSignalAnalysis
{
public:
    CANSignalAnalysis(); //constructor
    //All of these have message lenght and previous values as inputs. For many of them, if the message lenght is of the wrong size, the previous value is returned
    //The next inputs are the related bytes
    int batteryVoltage(int messageLength, int previousBatteryVoltage, byte dataByte1, byte dataByte0); //for batteryVoltage
    int batteryCurrent(int messageLength, int previousBatteryCurrent, byte dataByte3, byte dataByte2); //for batteryCurrent
    int rearTorque(int messageLength, int previousRearTorque, byte dataByte4, byte dataByte3); //for rear motor torque
    int minBattTemp(int messageLength, int previousMinBattTemp, byte dataByte6, byte dataByte5); //for minimum battery temperature
    int battCoolantRate(int messageLength, int previousBattCoolantRate, byte dataByte1, byte dataByte0); //for battery coolant rate
    int PTCoolantRate(int messageLength, int previousPTCoolantRate, byte dataByte3, byte dataByte2); //for power train coolant rate
    int maxRegen(int messageLength, int previousMaxRegen, byte dataByte1, byte dataByte0); //for maximum regen limit
    int maxDisChg(int messageLength, int previousMaxDisChg, byte dataByte3, byte dataByte2); //for maximum discharge limit
    int vehSpeed(int messageLength, int previousVehSpeed, byte dataByte2, byte dataByte1); //for vehicle UI speed
    int rightBlindSpot(int messageLength, int previousRightBlindSpot, byte dataByte0); //for right blind spot
    int leftBlindSpot(int messageLength, int previousLeftBlindSpot, byte dataByte0); //for left blind spot
private: //private variables corresponding to the above public variables
    int _messageLength;
    int _previousBatteryVoltage;
    int _previousBatteryCurrent;
    int _previousRearTorque;
    int _previousMinBattTemp;
    int _previousBattCoolantRate;
    int _previousPTCoolantRate;
    int _previousMaxRegen;
    int _previousMaxDisChg;
    int _previousVehSpeed;
    int _previousRightBlindSpot;
    int _previousLeftBlindSpot;
    byte _dataByte0;
    byte _dataByte1;
    byte _dataByte2;
    byte _dataByte3;
    byte _dataByte4;
    byte _dataByte5;
    byte _dataByte6;
   
    
};


#endif /* CANSignalAnalysis_h */
