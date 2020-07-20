#include "DisplayState.h"

CANServer::DisplayState *CANServer::DisplayState::display0 = new CANServer::DisplayState(0);
CANServer::DisplayState *CANServer::DisplayState::display1 = new CANServer::DisplayState(1);
CANServer::DisplayState *CANServer::DisplayState::display2 = new CANServer::DisplayState(2);


CANServer::DisplayState::DisplayState(const int displayId)
{
    _displayId = displayId;
    _displayOn = true;
}

CANServer::DisplayState::~DisplayState()
{

}

const char* CANServer::DisplayState::displayString() const
{
    if (_displayOn)
    {
        return "200vMNu30b0m120r";
    }
    else
    {
        //When the display is off we just set it to be text mode black space refresh 1sec
        return "1m t1000r";
    }
    
}

void CANServer::DisplayState::displayOn(const bool value)
{
    _displayOn = value;
}


/*
    if (DisplayOn == 1) {
        //set up display commands from data, see microDisplay command reference
        disp0mode = 0;
        /////disp0str = "-901vFDu0m11l"; //for display test
        disp0str = String(BattPower) + "vWK  Bu" + String(int(0.008 * BattPower)) + "b" + String(disp0mode) + "m" + "120r";
        disp1str = String(RearTorque) + "vMNu" + String(int(0.006*RearTorque)) + "b" + "0m120r";
        
        if (BSR != 0)   //Blind spot arrows over speed display
        {
          disp2str = "2v63488c6m120r";
        }
        else if (BSL != 0)
        {
            disp2str = "1v63488c6m120r";
        } else {
            if (SpeedUnit == 1) { //Convert to MPH
                disp2str = String(int(0.621371 * VehSpeed)) + "vHPMu" + String(int(0.008 * BattPower)) + "b0m120r";
            } else {   //speedunit = 1 for kph
                disp2str = String(int(VehSpeed)) + "vHPKu" + String(int(0.008 * BattPower)) + "b0m120r";
            }
        } //if BSR BSL
        
    } else if (DisplayOn == 0) {//turn all displays black if car screen is off
        disp0str = "1m t1000r"; //text mode black space refresh 1sec
        disp1str = "1m t1000r"; //text mode black space refresh 1sec
        disp2str = "1m t1000r"; //text mode black space refresh 1sec
    }
    */
