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
            log_i("Setting up SD Card...");

            if(!SD.begin())
            {
                log_e("Card Mount Failed");
                delay(500);
            }
            else
            {
                uint8_t cardType = SD.cardType();

                if (cardType == CARD_NONE)
                {
                    log_w("No SD card attached");
                    return;
                }

                uint64_t cardSize = SD.cardSize() / (1024 * 1024);
                log_i("SD Card Size: %lluMB", cardSize);
                log_i("Total space: %lluMB", SD.totalBytes() / (1024 * 1024));
                log_i("Used space: %lluMB", SD.usedBytes() / (1024 * 1024));

                _available = true;
            }

            log_i("Done");
        }

        const bool available()
        {
            return _available;
        }
    }
}