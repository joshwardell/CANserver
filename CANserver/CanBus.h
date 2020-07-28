#ifndef __CANBUS_H__
#define __CANBUS_H__

#define RAWCANLOGNAME "CAN.raw.log"

namespace CANServer 
{
    namespace CanBus 
    {
        void setup();
        void startup();
        void handle();
    }
}
#endif