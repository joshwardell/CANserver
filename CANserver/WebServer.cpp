#include "WebServer.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

//The SPIFFS editor taks up a lot of flash space.  For development purposes it is nice to have, but for production it should prob be comented out
#define INCLUDE_SPIFFS_EDITOR
#ifdef INCLUDE_SPIFFS_EDITOR
#include <SPIFFS.h>
#include <SPIFFSEditor.h>
#endif

#include "DisplayState.h"

AsyncWebServer server(80);

namespace CANServer
{
    namespace WebServer
    {
        void setup()
        {
            Serial.println("Setting up Web Server...");

        #ifdef INCLUDE_SPIFFS_EDITOR
            //Attach the SPIFFS editor helper so we can edit files on the fly
            server.addHandler(new SPIFFSEditor(SPIFFS, "admin","password"));
        #endif

            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/html/index.html");
            });

            server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/html/config.html");
            });

            server.on("/config/save", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                
                request->send(200);
            });

            server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/html/debug.html");
            });

            // set up servers for displays
            server.on("/disp0", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::DisplayState::display0->displayString());
            });
            server.on("/disp1", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::DisplayState::display1->displayString());
            });
            server.on("/disp2", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::DisplayState::display2->displayString());
            });
            
           

            //Serve our compressed version of zepto
            server.on("/js/zepto.min.js", HTTP_GET,  [](AsyncWebServerRequest *request){
                AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/zepto.min.js.gz", "text/javascript");
                response->addHeader("Content-Encoding", "gzip");
                response->addHeader("Cache-Control", "max-age=600");
                request->send(response);
            });

            //receive posts of display buttons, TODO do something with the buttons
            server.on("/post0", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {

                /*Serial.print("POST0: ");
                for (size_t i = 0; i < len; i++) {
                    Serial.write(data[i]);
                }
                Serial.println();*/
                request->send(200);
            });
            server.on("/post1", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                
                /*Serial.print("POST1: ");
                for (size_t i = 0; i < len; i++) {
                    Serial.write(data[i]);
                }
                Serial.println();*/
                request->send(200);
            });
            server.on("/post2", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                
                /*Serial.print("POST2: ");
                for (size_t i = 0; i < len; i++) {
                    Serial.write(data[i]);
                }
                Serial.println();*/

                request->send(200);
            });

            // Start server
            server.begin();

            Serial.println("Done");
        }
    }
}