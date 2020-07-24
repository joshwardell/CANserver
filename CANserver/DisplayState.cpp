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
            _displayString = "65535c%BattPower%vWK  Bu%BattPower_Scaled_Bar%b0m120r";
            break;
        }
        case 1:
        {
            _displayString = "65535c%RearTorque%vMNu%RearTorque_Scaled_Bar%b0m120r";
            break;
        }
        case 2:
        {
            _displayString = "65535c%VehSpeed%v%SpeedUnit%u%BattPower_Scaled_Bar%b0m120r";
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