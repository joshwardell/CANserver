//
//  generalCANSignalAnalysis.h
//  To be used with CANserver created by Josh Wardell
//
//  Created by Chris Allemang on July 4 2020.
//

#ifndef generalCANSignalAnalysis_h
#define generalCANSignalAnalysis_h

#include "Arduino.h"

class generalCANSignalAnalysis
{
  public:
    generalCANSignalAnalysis(); //constructor
    //function to analyze CAN signal, inputs: 64 bit CANMessage, start bit location (indexed starting at 0), singal bit lenght, signal factor, singal offset
    float getSignal(uint64_t CANMessage, int signalStartBit, int signalLength, int signalFactor, int signalOffset, bool ISsigned, bool byteOrder);
    float getSignal(uint64_t CANMessage, int signalStartBit, int signalLength, double signalFactor, int signalOffset, bool ISsigned, bool byteOrder);
  
  private: //private variables corresponding to the above public variables
    uint64_t _CANMessage;
    float _signalMessage;
    int _signalStartBit;
    int _signalLength;
    int _signalFactorInt;
    double _signalFactorDouble;
    int _signalOffset;
    bool _ISsigned;
    bool _byteOrder;
};


#endif /* generalCANSignalAnalysis_h */
