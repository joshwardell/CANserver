#include "SDCard.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

namespace CANServer
{
    namespace SDCard
    {
        bool _available = false;
        void setup()
        {
            Serial.println("Setting up SD Card...");

            if(!SD.begin())
            {
                Serial.println("Card Mount Failed");
                delay(500);
            }
            else
            {
                uint8_t cardType = SD.cardType();

                if (cardType == CARD_NONE)
                {
                    Serial.println("No SD card attached");
                    return;
                }
                
                Serial.printf("Total: %u MB, Used: %u MB\r\n", (size_t)(SD.totalBytes() / (1024 * 1024)), (size_t)(SD.usedBytes() / (1024 * 1024)));

                _available = true;
            }

            Serial.println("Done");
        }

        const bool available()
        {
            return _available;
        }
    }
}