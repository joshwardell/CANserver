#include "SPIFFileSystem.h"

#include <SPIFFS.h>


namespace CANServer
{
    namespace SPIFFileSystem
    {
        void setup()
        {
            Serial.println("Setting up SPI FS...");

            //Spin up access to the file system
            SPIFFS.begin();

            Serial.println("Done");
        }
    }
}