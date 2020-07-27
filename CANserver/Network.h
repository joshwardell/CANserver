#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <Arduino.h>

namespace CANServer 
{
    namespace Network 
    {
        void setup();
        void handle();

        void setExternalWifiSSID(const String value);
        void setExternalWifiPassword(const String value);

        const char* getExternalWifiSSID();
    }
}

#endif