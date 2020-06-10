//
//  CANSignalAnalysis.cpp
//  To be used with CANserver created by Josh Wardell
//
//  Created by Chris Allemang on Jun 9 2020.
//

#include "Arduino.h"
#include "CANSignalAnalysis.h"

CANSignalAnalysis::CANSignalAnalysis() //constructor
{
}

int CANSignalAnalysis::batteryVoltage(int messageLength, int previousBatteryVoltage, byte dataByte1, byte dataByte0)
{
  //set private variables
  _messageLength = messageLength;
  _previousBatteryVoltage = previousBatteryVoltage;
  _dataByte1 = dataByte1;
  _dataByte0 = dataByte0;
  //verify message is of correct length
  if (_messageLength == 8) {
    int _tempvolts;
    _tempvolts = 0.01 * float((_dataByte1 * 256) + _dataByte0); //divide by 100 and properly format byte1
    if ((_tempvolts > 290) && (_tempvolts < 420)) { //avoid some bad messages
      return (_tempvolts); //return voltage
    }
    //if the message is bad return the previous value
    else
    {
      return (_previousBatteryVoltage);
    }
  }
  //if the message lenght is incorrect return the previous value
  else
    return (_previousBatteryVoltage);
}

int CANSignalAnalysis::batteryCurrent(int messageLength, int previousBatteryCurrent, byte dataByte3, byte dataByte2)
{
  int _BattAmps;
  _messageLength = messageLength;
  _previousBatteryCurrent = previousBatteryCurrent;
  _dataByte3 = dataByte3;
  _dataByte2 = dataByte2;
  if (_messageLength == 8) {
    _BattAmps = (((_dataByte3 & 0x003F) << 8) + (_dataByte2)); //signed 15, mask off sign
    if (_BattAmps > 8190) {                 //15-bit signed conversion 16384/2
      _BattAmps = 0 - (16384 - _BattAmps);
    }
    _BattAmps = -0.1 * float(_BattAmps); //-0.1 scale
    return (_BattAmps); 
  }
  else
  {
    return (_previousBatteryCurrent);
  }
}

int CANSignalAnalysis::rearTorque(int messageLength, int previousRearTorque, byte dataByte4, byte dataByte3)
{
  int _tempTorque;
  _messageLength = messageLength;
  _previousRearTorque = previousRearTorque;
  _dataByte4 = dataByte4;
  _dataByte3 = dataByte3;
  if (_messageLength == 8) {
    _tempTorque = (((_dataByte4 & 0x000F) << 8) + (_dataByte3));
    if (_tempTorque > 2046) {                 //13-bit signed conversion 4096/2
      _tempTorque = 0 - (4096 - _tempTorque);
    }
    _tempTorque = 2.5 * float(_tempTorque);
    if ((_tempTorque < 5000) && (_tempTorque > -2000)) {  //reduce errors
      return (_tempTorque);
    }
  }
  else
  {
    return (_previousRearTorque);
  }
}

int CANSignalAnalysis::minBattTemp(int messageLength, int previousMinBattTemp, byte dataByte6, byte dataByte5)
{
  _messageLength = messageLength;
  _previousMinBattTemp = previousMinBattTemp;
  _dataByte6 = dataByte6;
  _dataByte5 = dataByte5;
  if (_messageLength == 8) {
    return (int((float((((_dataByte6 << 8) + _dataByte5) & 0x1FF0) >> 4) * .25 - 25) * 1.8 + 32));

  }
  else
  {
    return (_previousMinBattTemp);
  }
}

int CANSignalAnalysis::battCoolantRate(int messageLength, int previousBattCoolantRate, byte dataByte1, byte dataByte0)
{
  _messageLength = messageLength;
  _previousBattCoolantRate = previousBattCoolantRate;
  _dataByte1 = dataByte1;
  _dataByte0 = dataByte0;
  return (0.1 * float(((_dataByte1 << 8) + _dataByte0) & 0x01FF));
}

int CANSignalAnalysis::PTCoolantRate(int messageLength, int previousPTCoolantRate, byte dataByte3, byte dataByte2)
{
  _messageLength = messageLength;
  _previousPTCoolantRate = previousPTCoolantRate;
  _dataByte3 = dataByte3;
  _dataByte2 = dataByte2;
  return (0.1 * float((((_dataByte3 << 8) + _dataByte2) & 0x07FC0)));
}

int CANSignalAnalysis::maxRegen(int messageLength, int previousMaxRegen, byte dataByte1, byte dataByte0)
{
  _messageLength = messageLength;
  _previousMaxRegen = previousMaxRegen;
  _dataByte1 = dataByte1;
  _dataByte0 = dataByte0;
  return (0.01 * float((_dataByte1 * 256) + _dataByte0));
}

int CANSignalAnalysis::maxDisChg(int messageLength, int previousMaxDisChg, byte dataByte3, byte dataByte2)
{
  _messageLength = messageLength;
  _previousMaxDisChg = previousMaxDisChg;
  _dataByte3 = dataByte3;
  _dataByte2 = dataByte2;
  return (0.01 * float((_dataByte3 * 256) + _dataByte2));
}

int CANSignalAnalysis::vehSpeed(int messageLength, int previousVehSpeed, byte dataByte2, byte dataByte1)
{
  _messageLength = messageLength;
  _previousVehSpeed = previousVehSpeed;
  _dataByte2 = dataByte2;
  _dataByte1 = dataByte1;
  if (_messageLength == 8) {
    return (0.8 * float((((dataByte2 << 8) + dataByte1) & 0xFFF0) >> 4) - 400);
  }
  else
  {
    return (_previousVehSpeed);
  }
}
int CANSignalAnalysis::rightBlindSpot(int messageLength, int previousRightBlindSpot, byte dataByte0)
{
  _messageLength = messageLength;
  _previousRightBlindSpot = previousRightBlindSpot;
  _dataByte0 = dataByte0;
  if (_messageLength == 8) {
    int _BSR;
    _BSR = (dataByte0 & 0xC0) >> 6;
    if (_BSR > 2) {  // 3 is active but no warning
      return (0);
    }
  }
  else
  {
    return (previousRightBlindSpot);
  }
}

int CANSignalAnalysis::leftBlindSpot(int messageLength, int previousLeftBlindSpot, byte dataByte0)
{
  _messageLength = messageLength;
  _previousLeftBlindSpot = previousLeftBlindSpot;
  _dataByte0 = dataByte0;
  if (_messageLength == 8) {
    int _BSL;
    _BSL = (dataByte0 & 0x30) >> 4;
    if (_BSL > 2) {  // 3 is active but no warning
      return (0);
    }
  }
  else
  {
    return (_previousLeftBlindSpot);
  }
}
