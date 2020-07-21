#include "DisplayState.h"
#include "VehicleState.h"

#include <SPIFFS.h>

CANServer::DisplayState *CANServer::DisplayState::display0 = new CANServer::DisplayState(0);
CANServer::DisplayState *CANServer::DisplayState::display1 = new CANServer::DisplayState(1);
CANServer::DisplayState *CANServer::DisplayState::display2 = new CANServer::DisplayState(2);


CANServer::DisplayState::DisplayState(const int displayId)
{
    _displayId = displayId;
}

CANServer::DisplayState::~DisplayState()
{

}


void CANServer::DisplayState::updateDisplayString(const char* newValue)
{
    _displayString = newValue;
}

String _settingsFileName(const int id)
{
    String fileName = String("/settings/display") + id + String(".conf");

    return fileName;
}

void CANServer::DisplayState::load()
{
    String filename = _settingsFileName(_displayId);
    if (SPIFFS.exists(filename))
    {
        File file = SPIFFS.open(filename, FILE_READ);
        if (file)
        {
            _displayString = file.readString();

            file.close();

            //We managed to load the settings for this display.  We can return now
            return;
        }
    }
    
    //Couldn't open the settings file for this display.  Default it
    switch(_displayId)
    {
        case 0:
        {
            _displayString = "%BattPower%vWK  Bu%BattPower%b0m120r";
            break;
        }
        case 1:
        {
            _displayString = "%RearTorque%vMNu%RearTorque%b0m120r";
            break;
        }
        case 2:
        {
            _displayString = "%VehSpeed%vHPKu%BattPower%b0m120r";
            break;
        }
    }
}

void CANServer::DisplayState::save()
{
    File file = SPIFFS.open(_settingsFileName(_displayId), FILE_WRITE);
    if (file)
    {
        file.print(_displayString);
        file.close();
    }
}
/*{
    CANServer::VehicleState *vehicleState = CANServer::VehicleState::instance();
    if (vehicleState->DisplayOn)
    {
        //Temporary code to return functionality to the existing way
        switch (_displayId)
        {
            case 0:
            {
                _displayString = (String("%BattPower%vWK  Bu") + String(int(0.008 * vehicleState->BattPower)) + "b0m120r");
                break;
            }
            case 1:
            {
                _displayString = (String("%RearTorque%vMNu") + String(int(0.006*vehicleState->RearTorque)) + "b0m120r");
                break;
            }
            case 2:
            {
                if (vehicleState->BSR != 0)   //Blind spot arrows over speed display
                {
                    _displayString = "2v63488c6m120r";
                }
                else if (vehicleState->BSL != 0)
                {
                    _displayString = "1v63488c6m120r";
                } else {
                    //if (SpeedUnit == 1) { //Convert to MPH
                    //    disp2str = String(int(0.621371 * VehSpeed)) + "vHPMu" + String(int(0.008 * BattPower)) + "b0m120r";
                    //} else {   //speedunit = 1 for kph
                    _displayString = (String(int(vehicleState->VehSpeed)) + "vHPKu" + String(int(0.008 * vehicleState->BattPower)) + "b0m120r");
                    //}
                } //if BSR BSL

                break;
            }
        }
    }
    else
    {
        //When the display is off we just set it to be text mode black space refresh 1sec
        _displayString = offDisplayString();
    }
}*/

const char* CANServer::DisplayState::displayString() const 
{
    return _displayString.c_str();
}

const uint CANServer::DisplayState::displayStringLength() const
{
    return _displayString.length();
}


void CANServer::DisplayState::loadAll()
{
    display0->load();
    display1->load();
    display2->load();
}