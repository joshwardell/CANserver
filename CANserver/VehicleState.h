#ifndef __VEHICLESTATE_H__
#define __VEHICLESTATE_H__

#include <stdlib.h>
namespace CANServer
{
    class VehicleState
    {
    public:
        static VehicleState* instance();
        ~VehicleState();

        int BattVolts;        //ID 132 byte 0+1 scale .01 V
        int BattAmps;         //ID 132 byte 2+3 scale -.1 offset 0 A
        int BattPower;        //V*A
        int RearTorque;       //ID 1D8 startbit 24 signed13 scale 0.25 NM
        int FrontTorque;      //ID 1D8 startbit 24 signed13 scale 0.25 NM
        int MinBattTemp;      //ID 312 SB 44 u9 scale .25 offset -25 C
        int BattCoolantRate;  //ID 241 SB 0 u9 scale .01 LPM
        int PTCoolantRate;    //ID 241 SB 22 u9 scale .01 LPM
        int MaxRegen;         //ID 252 Bytes 0+1 scale .01 kW
        int MaxDisChg;        //ID 252 Bytes 2+3 scale .01 kW
        int VehSpeed;         //ID 257 SB 12 u12 scale .08 offset -40 KPH
        int SpeedUnit;        //ID 257
        int v12v261;          //ID 261 SB0 u12 scale 0.005444 V
        int BattCoolantTemp;  //ID 321 SB0 u10 scale 0.125 offset -40 C
        int PTCoolantTemp;    //ID 321 SB10 u10 scale 0.125 offset -40 C
        int BattRemainKWh;    //ID 352 byte 1/2 scale .1 kWh
        int BattFullKWh;      //ID 352 byte 0/1 scale .1 kWh
        int InvHStemp376;     //ID 376 Byte 1 scale .5 offset -20 C
        int BSR;
        int BSL;
        int DisplayOn;       //to turn off displays if center screen is off

    private:
        VehicleState();

        static VehicleState *_instance;
    };
}

#endif
