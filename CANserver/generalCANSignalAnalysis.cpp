//
//  generalCANSignalAnalysis.cpp
//  To be used with CANserver created by Josh Wardell
//
//  Created by Chris Allemang on July 4 2020.
//

#include "Arduino.h"
#include "generalCANSignalAnalysis.h"


generalCANSignalAnalysis::generalCANSignalAnalysis() //constructor
{
}

float generalCANSignalAnalysis::getSignal(uint64_t CANMessage, int signalStartBit, int signalLength, int signalFactor, int signalOffset, bool ISsigned, bool byteOrder)
{
    _CANMessage = CANMessage;
    _signalStartBit = signalStartBit;
    _signalLength = signalLength;
    _signalFactorInt = signalFactor;
    _signalOffset = signalOffset;
    _ISsigned = ISsigned;
    _byteOrder = byteOrder;
    if (_byteOrder == 1){
        _signalMessage = ((1 << _signalLength) - 1) & (_CANMessage >> _signalStartBit); //mask unwanted bits and shfit
        if (_ISsigned == true){
            if(_signalMessage > pow(2, _signalLength-1))
            {
                _signalMessage = 0 - (pow(2, _signalLength) - _signalMessage);
            }
            _signalMessage = signed(_signalMessage) * _signalFactorDouble + _signalOffset; //apply factor and offset
        }
        else
        {
            _signalMessage = unsigned(_signalMessage) * _signalFactorInt + _signalOffset; //apply factor and offset
        }
    }
    return (_signalMessage); //return analyzed signal
}

float generalCANSignalAnalysis::getSignal(uint64_t CANMessage, int signalStartBit, int signalLength, double signalFactor, int signalOffset, bool ISsigned, bool byteOrder)
{
    _CANMessage = CANMessage;
    _signalStartBit = signalStartBit;
    _signalLength = signalLength;
    _signalFactorDouble = signalFactor;
    _signalOffset = signalOffset;
    _ISsigned = ISsigned;
    _byteOrder = byteOrder;
    if (_byteOrder == 1){
        _signalMessage = ((1 << _signalLength) - 1) & (_CANMessage >> _signalStartBit); //mask unwanted bits and shfit
        if (_ISsigned == true){
            if(_signalMessage > pow(2, _signalLength-1))
            {
                _signalMessage = 0 - (pow(2, _signalLength) - _signalMessage);
            }
            _signalMessage = float(signed(_signalMessage)) * _signalFactorDouble + _signalOffset; //apply factor and offset
        }
        else
        {
            _signalMessage = float(unsigned(_signalMessage)) * _signalFactorDouble + _signalOffset;
        }
    }
    return (_signalMessage); //return analyzed signal
}
