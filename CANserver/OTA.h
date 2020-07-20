#ifndef __OTA_H__
#define __OTA_H__

namespace CANServer
{
    namespace OTA
    {
        void setup(const char* hostname, const char* password);
        void handle();
    }
}

#endif