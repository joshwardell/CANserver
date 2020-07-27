#include "OTA.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

const char* otaHostname = "CANserver";
const char* otaPassword = "JWcanServer2020";

namespace CANServer
{
    namespace OTA
    {
        void setup()
        {
            Serial.println("Setting up OTA Updates...");

            // set up ArduinoOTA
            ArduinoOTA.setHostname(otaHostname);
            ArduinoOTA.setPassword(otaPassword);
            
            ArduinoOTA.onStart([]() {
                String type;

                //Don't service any http requests while updating
                server.end();
                if (ArduinoOTA.getCommand() == U_FLASH)
                {
                    type = "sketch";
                }
                else // U_SPIFFS
                {
                    type = "filesystem";

                    //Shut down access to the SPIFFS filesystem
                    SPIFFS.end();
                }

                Serial.println("Start updating " + type);
            });

            ArduinoOTA.onEnd([]() {
                Serial.println("ArduinoOTA: End\n");
                delay(500);

                server.begin(); //Resume server after update is finished
                SPIFFS.begin(); //Resume access to the SPIFFS filesystem
            });

            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            });

            ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
                Serial.println("\n");
            });
            
            ArduinoOTA.begin();
            
            Serial.println("Done");
        }

        void handle()
        {
            ArduinoOTA.handle();
        }
    }
}