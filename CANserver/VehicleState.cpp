#include "VehicleState.h"

CANServer::VehicleState* CANServer::VehicleState::_instance = NULL;

CANServer::VehicleState* CANServer::VehicleState::instance()
{
    if (_instance == NULL)
    {
        _instance = new CANServer::VehicleState();
    }

    return _instance;
}

CANServer::VehicleState::VehicleState()
{
    BattVolts = 0;
    BattAmps = 0;
    BattPower = 0;
    RearTorque = 0;
    FrontTorque = 0;
    MinBattTemp = 0;
    BattCoolantRate = 0;
    PTCoolantRate = 0;
    MaxRegen = 0;
    MaxDisChg = 0;
    VehSpeed = 0;
    SpeedUnit = 0;
    v12v261 = 0;
    BattCoolantTemp = 0;
    PTCoolantTemp = 0;
    BattRemainKWh = 0;
    BattFullKWh = 0;
    InvHStemp376 = 0;
    BSR = 0;
    BSL = 0;
    DisplayOn = 1;
}

CANServer::VehicleState::~VehicleState()
{
    
}