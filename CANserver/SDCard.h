#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <SD.h>

namespace CANServer
{
    namespace SDCard
    {        
        void setup();
        const bool available();
    }
}

#endif