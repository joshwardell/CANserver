#include "SerialPorts.h"

#include <Arduino.h>

namespace CANServer
{
    namespace SerialPorts
    {
        void setup()
        {
            Serial.begin(57600);
            //Serial1.begin(57600, SERIAL_8N1, 32, 10);
        }

        void handle()
        {
        }
    }
}
