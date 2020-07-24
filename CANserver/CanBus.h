#ifndef __CANBUS_H__
#define __CANBUS_H__

#define RAWCANLOGNAME "CAN.raw.log"

namespace CANServer 
{
    namespace CanBus 
    {
        extern bool logRawCan;
        void setup();
        void startup();
        void handle();

        void saveSettings();

        void openRawLog();
        void closeRawLog();
    }
}
#endif