#include "DisplayState.h"
#include "VehicleState.h"

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

void CANServer::DisplayState::update()
{
    CANServer::VehicleState *vehicleState = CANServer::VehicleState::instance();
    if (vehicleState->DisplayOn)
    {
        //Temporary code to return functionality to the existing way
        switch (_displayId)
        {
            case 0:
            {
                _displayString = (String(vehicleState->BattPower) + "vWK  Bu" + String(int(0.008 * vehicleState->BattPower)) + "b0m120r");
                break;
            }
            case 1:
            {
                _displayString = (String(vehicleState->RearTorque) + "vMNu" + String(int(0.006*vehicleState->RearTorque)) + "b0m120r");
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
                } //if BSR BSL*/

                break;
            }
        }
    }
    else
    {
        //When the display is off we just set it to be text mode black space refresh 1sec
        _displayString = "1m t1000r";
    }
}

const char* CANServer::DisplayState::displayString() const 
{
    return _displayString.c_str();
}
