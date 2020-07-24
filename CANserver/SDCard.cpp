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

                uint64_t cardSize = SD.cardSize() / (1024 * 1024);
                Serial.printf("SD Card Size: %lluMB\r\n", cardSize);
                Serial.printf("Total space: %lluMB\r\n", SD.totalBytes() / (1024 * 1024));
                Serial.printf("Used space: %lluMB\r\n", SD.usedBytes() / (1024 * 1024));

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