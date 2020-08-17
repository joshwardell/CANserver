#include "WebServer.h"

#include <Arduino.h>
#include <esp_wifi.h>
#include "esp_async_webserver/ESPAsyncWebServer.h"
#include <Update.h>

#include <iostream>
#include <iomanip>
#include <sstream>

//The SPIFFS editor taks up a lot of flash space.  For development purposes it is nice to have, but for production it should prob be comented out
#ifdef INCLUDE_SPIFFS_EDITOR
#include "esp_async_webserver/SPIFFSEditor.h"
#endif

#include "esp_async_webserver/AsyncJson.h"
#include <SD.h>

#include "BuildRev.h"
#include "Average.h"
#include "SDCard.h"
#include "CanBus.h"
#include "Network.h"
#include "Logging.h"
#include "Displays.h"
#include "SPIFFileSystem.h"
#include "PandaUDP.h"

bool RebootAfterUpdate = false;
bool PauseAllProcessing = false;

//Make sure the compiler knows this exists in our memory space
extern Average<uint32_t> _memoryUsage;
extern PandaUDP panda;

#define ASSIGN_HELPER(keyname) if (keyParam->value() == #keyname)\
                    {\
                        vehicleStateInstance->keyname = atoi(valParam->value().c_str());\
                    }
#define FETCH_HELPER(keyname) vehiclestatus[#keyname] = vehicleStateInstance->keyname;
AsyncWebServer server(80);

String _buildInfo(const String& var)
{
    if(var == "BUILD_DATE")
        return F(__DATE__ " " __TIME__);

    if(var == "BUILD_REV")
        return F("" BUILD_REV);

    if (var == "SPIFFS_REV")
    {
        String revString = "unknown";
        File spiffsrevFile = CANServer::SPIFFileSystem::SPIFFS_app.open("/rev", "r");
        if (spiffsrevFile)
        {
            revString = spiffsrevFile.readString();
            spiffsrevFile.close();
        }
        
        return revString;
    }

    return String();
}

namespace CANServer
{
    namespace WebServer
    {

        void _handleUpdateChunk(const int updateType, const unsigned long paritionSize, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

        CANServer::CanBus::AnalysisItemMap::const_iterator analysisLoadIterator;
        uint8_t analysisLoadOutputState = 0;
        bool analysisLoadInitialOutput = false;
        bool analysisLoadFinalOutput = false;


        void setup()
        {
            Serial.println(F("Setting up Web Server..."));

        #ifdef INCLUDE_SPIFFS_EDITOR
            //Attach the SPIFFS editor helper so we can edit files on the fly
            server.addHandler(new SPIFFSEditor(CANServer::SPIFFileSystem::SPIFFS_app, "admin","password"));
        #endif

            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(CANServer::SPIFFileSystem::SPIFFS_app, "/html/index.html", String(), false, _buildInfo);
            });

            server.on("/memory", HTTP_GET, [](AsyncWebServerRequest *request){
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                //log_v("RAM Usage: mean: %0.2f, max: %d, min: %d, stdev: %0.2f", _memoryUsage.mean(), _memoryUsage.maximum(), _memoryUsage.minimum(), _memoryUsage.stddev());    

                doc["mean"] = _memoryUsage.mean();
                doc["max"] = _memoryUsage.maximum();
                doc["min"] = _memoryUsage.minimum();
                doc["stddev"] = _memoryUsage.stddev();

                response->setLength();
                request->send(response);
            });





            //Display configuration related url handling
            server.on("/displays", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(CANServer::SPIFFileSystem::SPIFFS_app, "/html/displays.html", String(), false, _buildInfo);
            });

            server.on("/display_load", HTTP_GET, [](AsyncWebServerRequest *request) {
                
                if(request->hasParam("dispId", false))
                {
                    uint8_t displayId = atoi(request->getParam("dispId", false)->value().c_str());
                    if (displayId > 3 || displayId < 0)
                    {
                        request->send(404);
                        return;
                    }

                    request->send(CANServer::SPIFFileSystem::SPIFFS_data, CANServer::Displays::instance()->filenameForDisplay(displayId));
                }
                else
                {
                    request->send(404);
                }
            }); 

            server.on("/display_stats", HTTP_GET, [](AsyncWebServerRequest *request) {
                if(request->hasParam("dispId", false))
                {
                    uint8_t displayId = atoi(request->getParam("dispId", false)->value().c_str());
                    if (displayId > 3 || displayId < 0)
                    {
                        request->send(404);
                        return;
                    }

                    AsyncJsonResponse * response = new AsyncJsonResponse();
                    JsonVariant& doc = response->getRoot();

                    CANServer::Displays *displaysInstance = CANServer::Displays::instance();

                    doc["state"] = !displaysInstance->scriptErrorForDisplay(displayId);
                    doc["errorstring"] = displaysInstance->errorStringForDisplay(displayId);

                    Average<uint16_t>* processingTime = displaysInstance->processingTimeForDisplay(displayId);
                    doc["mean"] = processingTime->mean();
                    doc["mode"] = processingTime->mode();
                    doc["max"] = processingTime->maximum();
                    doc["min"] = processingTime->minimum();
                    doc["stddev"] = processingTime->stddev();
                    
                    response->setLength();
                    request->send(response);
                }
                else
                {
                    request->send(404);
                }
            }); 

            server.on("/display_save", HTTP_POST, [](AsyncWebServerRequest *request) {
                if(request->hasParam("script", true) && request->hasParam("dispId", true))
                {
                    uint8_t displayId = atoi(request->getParam("dispId", true)->value().c_str());
                    if (displayId > 3 || displayId < 0)
                    {
                        request->send(404);
                        return;
                    }

                    CANServer::Displays::instance()->saveScriptForDisplay(displayId, request->getParam("script", true)->value().c_str());
                    CANServer::Displays::instance()->loadScriptForDisplay(displayId);

                    request->send(200);
                    return;
                }

                request->send(404);
                return;
            });




            //Network configuration related url handling
            server.on("/network", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(CANServer::SPIFFileSystem::SPIFFS_app, "/html/network.html", String(), false, _buildInfo);
            });

            server.on("/network_save", HTTP_POST, [](AsyncWebServerRequest * request) {
                
                if(request->hasParam("ssid", true))
                {
                    AsyncWebParameter* newValue = request->getParam("ssid", true);
                    CANServer::Network::setExternalWifiSSID(newValue->value());
                }
                else
                {
                    CANServer::Network::setExternalWifiPassword("");
                }
                
                if(request->hasParam("password", true))
                {
                    AsyncWebParameter* newValue = request->getParam("password", true);
                    CANServer::Network::setExternalWifiPassword(newValue->value());
                }
                else
                {
                    CANServer::Network::setExternalWifiPassword("");
                }

                request->redirect("/network");
            });

            server.on("/network_update", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                JsonObject networksettings = doc.createNestedObject("networksettings");
                
                networksettings["ssid"] = CANServer::Network::getExternalWifiSSID();               

                response->setLength();
                request->send(response);
            }); 

            server.on("/network_stationlist", HTTP_GET, [](AsyncWebServerRequest *request) {

                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                wifi_sta_list_t wifi_sta_list;
                tcpip_adapter_sta_list_t adapter_sta_list;
                
                memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
                memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
                
                esp_wifi_ap_get_sta_list(&wifi_sta_list);
                tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
                
                JsonArray stationsList = doc.createNestedArray("stations"); 
                std::stringstream macStringStream;
                for (int i = 0; i < adapter_sta_list.num; i++) {
                
                    JsonObject stationInfo = stationsList.createNestedObject(); 

                    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
                
                    macStringStream.str("");
                    macStringStream.clear();

                    macStringStream << std::setw(2) << std::setfill('0') << std::uppercase << std::hex;
                    for(int i = 0; i< 6; i++)
                    {
                        if (i > 0) { macStringStream << ":"; }
                        macStringStream << (uint)(station.mac[i]);
                    }
    
                    stationInfo["mac"] = macStringStream.str();
                    stationInfo["ip"] = ip4addr_ntoa(&(station.ip));
                }      

                response->setLength();
                request->send(response);                
            });

            server.on("/network_status", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                doc["externalStatus"] = CANServer::Network::getExternalStatus();
                if (CANServer::Network::getExternalStatus()) {
                    doc["externalIP"] = WiFi.localIP().toString();
                }

                JsonObject pandaDetails = doc.createNestedObject("panda");
                pandaDetails["ip"] = panda.remoteIP()->toString();
                pandaDetails["port"] = panda.remotePort();

                response->setLength();
                request->send(response);        
            });




            //Accessability to log files
            server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send(CANServer::SPIFFileSystem::SPIFFS_app, "/html/logs.html", String(), false, _buildInfo);
            }); 
#define LOGSettingsUpdateHelper(postvarname, logtype) {\
JsonObject detailsNode = doc.createNestedObject(postvarname);\
detailsNode["enabled"] = logginginstance->isActive(logtype);\
detailsNode["hasfile"] = logginginstance->path(logtype).length() > 0;\
detailsNode["filesize"] = logginginstance->fileSize(logtype);\
}
            server.on("/logs_update", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                CANServer::Logging *logginginstance = CANServer::Logging::instance();
                
                LOGSettingsUpdateHelper("rawlog", CANServer::Logging::LogType_Raw);
                LOGSettingsUpdateHelper("intervallog", CANServer::Logging::LogType_Interval);
                LOGSettingsUpdateHelper("drivelog", CANServer::Logging::LogType_Drive);
                LOGSettingsUpdateHelper("chargelog", CANServer::Logging::LogType_Charge);
                LOGSettingsUpdateHelper("seriallog", CANServer::Logging::LogType_Serial);
                
                JsonObject sdDetailsNode = doc.createNestedObject("sddetails");
                sdDetailsNode["available"] = SDCard::available();
                if (SDCard::available())
                {
                    sdDetailsNode["totalmbytes"] = (uint32_t)(SD.totalBytes() / (1024 * 1024));
                    sdDetailsNode["usedmbytes"] = (uint32_t)(SD.usedBytes() / (1024 * 1024));
                }

                response->setLength();
                request->send(response);
            });     

            server.on("/log_download", HTTP_GET, [](AsyncWebServerRequest *request) {

                if(request->hasParam("id", false))
                {
                    AsyncWebParameter* logid = request->getParam("id", false);

                    CANServer::Logging *logginginstance = CANServer::Logging::instance();

                    CANServer::Logging::LogType logType = CANServer::Logging::LogType_Unknown;
                    if (logid->value() == "rawlog")
                    {
                        logType = CANServer::Logging::LogType_Raw;
                    }
                    else if (logid->value() == "intervallog")
                    {
                        logType = CANServer::Logging::LogType_Interval;
                    }
                    else if (logid->value() == "drivelog")
                    {
                        logType = CANServer::Logging::LogType_Drive;
                    }
                    else if (logid->value() == "chargelog")
                    {
                        logType = CANServer::Logging::LogType_Charge;
                    }

                    if (logType != CANServer::Logging::LogType_Unknown)
                    {
                        //Disable before download and enable (if enabled) after
                        bool isEnabled = logginginstance->isActive(logType);
                        logginginstance->disable(logType);

                        request->send(SD, logginginstance->path(logType), "application/octet-stream", true);

                        if (isEnabled)
                        {
                            logginginstance->enable(logType);
                        }
                        return;
                    }
                }
                
                //If we got to here we can't find the log that was asked for
                request->send(404);
            }); 

            server.on("/log_delete", HTTP_GET, [](AsyncWebServerRequest *request) {

                if(request->hasParam("id", false))
                {
                    AsyncWebParameter* logid = request->getParam("id", false);

                    CANServer::Logging::LogType logType = CANServer::Logging::LogType_Unknown;
                    if (logid->value() == "rawlog")
                    {
                        logType = CANServer::Logging::LogType_Raw;
                    }
                    else if (logid->value() == "intervallog")
                    {
                        logType = CANServer::Logging::LogType_Interval;
                    }
                    else if (logid->value() == "drivelog")
                    {
                        logType = CANServer::Logging::LogType_Drive;
                    }
                    else if (logid->value() == "chargelog")
                    {
                        logType = CANServer::Logging::LogType_Charge;
                    }

                    if (logType != CANServer::Logging::LogType_Unknown)
                    {
                        CANServer::Logging::instance()->deleteFile(logType);                        
                    }
                }
                
                request->redirect("/logs");
            }); 

#define LOGSettingsSaveHelper(postvarname, logtype) {\
bool requestedRawLogState = false; \
if (request->hasParam(postvarname, true))\
{\
    AsyncWebParameter* newValue = request->getParam(postvarname, true);\
    if (newValue->value() == "on")\
    {\
        requestedRawLogState = true;\
    }\
    else\
    {\
        requestedRawLogState = false;\
    }\
}\
\
if (requestedRawLogState)\
{\
    logginginstance->enable(logtype);\
}\
else\
{\
    logginginstance->disable(logtype);\
}\
}
            server.on("/logs_save", HTTP_POST, [](AsyncWebServerRequest * request) {
                
                CANServer::Logging *logginginstance = CANServer::Logging::instance();

                LOGSettingsSaveHelper("rawlog", CANServer::Logging::LogType_Raw);
                LOGSettingsSaveHelper("intervallog", CANServer::Logging::LogType_Interval);
                LOGSettingsSaveHelper("drivelog", CANServer::Logging::LogType_Drive);
                LOGSettingsSaveHelper("chargelog", CANServer::Logging::LogType_Charge);
                LOGSettingsSaveHelper("seriallog", CANServer::Logging::LogType_Serial);

                CANServer::Logging::instance()->saveConfiguraiton();

                request->redirect("/logs");
            });    




            //Dynamic Analysis configuration
            server.on("/analysis", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(CANServer::SPIFFileSystem::SPIFFS_app, "/html/analysis.html", String(), false, _buildInfo);
            });

            server.on("/analysis_delete", HTTP_POST, [](AsyncWebServerRequest * request) {
                if(request->hasParam("name", true))
                {
                    std::string itemName = request->getParam("name", true)->value().c_str();
                    bool didDelete = false;

                    CANServer::CanBus *canbusInstance = CANServer::CanBus::instance();

                    canbusInstance->pauseDynamicAnalysis();
                   
                    CANServer::CanBus::AnalysisItemMap::iterator it = canbusInstance->dynamicAnalysisItems()->find(itemName);
                    if (it != canbusInstance->dynamicAnalysisItems()->end())
                    {
                        delete it->second;
                        canbusInstance->dynamicAnalysisItems()->erase(itemName.c_str());
                        didDelete = true;
                    }                        
                    
                    if (didDelete)
                    {
                        //Clean up the file for this item
                        canbusInstance->deleteDynamicAnalysisFile(itemName.c_str());

                        canbusInstance->resolveLookups();
                    }
                    canbusInstance->resumeDynamicAnalysis();

                    request->send(200); 
                }
                else
                {
                   request->send(404); 
                }
            });
             

            server.on("/analysis_save", HTTP_POST, [](AsyncWebServerRequest * request) {                
/*
state: update
origName: TestVolts1
name: TestVolts
frameid: 306
startbit: 0
bitlength: 16
factor: 0.01
signaloffset: 0
issigned: false
littleendian: true
*/
                if(request->hasParam("state", true) &&
                    request->hasParam("origName", true) &&
                    request->hasParam("name", true) &&
                    request->hasParam("frameid", true) &&
                    request->hasParam("startbit", true) &&
                    request->hasParam("bitlength", true) &&
                    request->hasParam("factor", true) &&
                    request->hasParam("signaloffset", true))
                {
                    std::string itemName = request->getParam("name", true)->value().c_str();
                    std::string origItemName = request->getParam("origName", true)->value().c_str();

                    CANServer::CanBus::AnalysisItem *analysisItem = new CANServer::CanBus::AnalysisItem();
                    analysisItem->frameId = atoi(request->getParam("frameid", true)->value().c_str());
                    analysisItem->startBit = atoi(request->getParam("startbit", true)->value().c_str());
                    analysisItem->bitLength = atoi(request->getParam("bitlength", true)->value().c_str());
                    analysisItem->factor = atof(request->getParam("factor", true)->value().c_str());
                    analysisItem->signalOffset = atoi(request->getParam("signaloffset", true)->value().c_str());

                    if (request->hasParam("issigned", true) && request->getParam("issigned", true)->value() == "true")
                    {
                        analysisItem->isSigned = true;
                    }
                    else
                    {
                        analysisItem->isSigned = false;
                    }

                    if (request->hasParam("littleendian", true) && request->getParam("littleendian", true)->value() == "true")
                    {
                        analysisItem->byteOrder = true;
                    }
                    else
                    {
                        analysisItem->byteOrder = false;
                    }

                    if (request->hasParam("drivelog", true) && request->getParam("drivelog", true)->value() == "true")
                    {
                        analysisItem->driveLog = true;
                    }
                    else
                    {
                        analysisItem->driveLog = false;
                    }

                    if (request->hasParam("chargelog", true) && request->getParam("chargelog", true)->value() == "true")
                    {
                        analysisItem->chargeLog = true;
                    }
                    else
                    {
                        analysisItem->chargeLog = false;
                    }

                    //Pause running of the items so we can modify them
                    CANServer::CanBus *canbusInstance = CANServer::CanBus::instance();
                    canbusInstance->pauseDynamicAnalysis();

                    if(request->hasParam("state", true) && request->getParam("state", true)->value() == "update")
                    {
                        //We use the origItemName field here just incase the user has changed the name.  The orig name comes from the HTML dom.

                        //Clean up the old entry so we can replace it
                        CANServer::CanBus::AnalysisItemMap::iterator it = canbusInstance->dynamicAnalysisItems()->find(origItemName);
                        if (it != canbusInstance->dynamicAnalysisItems()->end())
                        {
                            delete it->second;
                            canbusInstance->dynamicAnalysisItems()->erase(origItemName);

                            canbusInstance->deleteDynamicAnalysisFile(origItemName.c_str());
                        }                        
                    }

                    //insert our new entry
                    canbusInstance->dynamicAnalysisItems()->insert(CANServer::CanBus::AnalysisItemPair(itemName, analysisItem));

                    canbusInstance->resolveLookups();

                    //Start processing back up again
                    canbusInstance->resumeDynamicAnalysis();

                    //Save off the config file for this item
                    canbusInstance->saveDynamicAnalysisFile(itemName.c_str());
                }
                else
                {
                    //We didn't have all the params we needed.  404 for you
                    request->send(404);
                }

                request->send(200);
            });
            
            server.on("/analysis_load", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                for (CANServer::CanBus::AnalysisItemMap::const_iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->begin(); it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end(); it++)
                {
                    doc[it->first.c_str()] = 1;
                }

                response->setLength();
                request->send(response);
            });

            server.on("/analysis_info", HTTP_GET, [](AsyncWebServerRequest *request) {
                if(request->hasParam("item", false))
                {
                    CANServer::CanBus::AnalysisItemMap::const_iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->find(request->getParam("item", false)->value().c_str());
                    if (it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end())
                    {
                        AsyncJsonResponse * response = new AsyncJsonResponse();
                        JsonVariant& doc = response->getRoot();

                        doc["frameid"] = it->second->frameId;
                        doc["startBit"] = it->second->startBit;
                        doc["bitLength"] = it->second->bitLength;
                        doc["factor"] = it->second->factor;
                        doc["signalOffset"] = it->second->signalOffset;
                        doc["isSigned"] = it->second->isSigned;
                        doc["byteOrder"] = it->second->byteOrder;
                        doc["driveLog"] = it->second->driveLog;
                        doc["chargeLog"] = it->second->chargeLog;

                        response->setLength();
                        request->send(response);

                        return;
                    }
                }

                request->send(404);
            });

            server.on("/analysis_update", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                for (CANServer::CanBus::AnalysisItemMap::const_iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->begin(); it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end(); it++)
                {
                    CANServer::CanBus::AnalysisItem *analysisItem = it->second;
                    doc[it->first] = analysisItem->lastValue;
                }

                response->setLength();
                request->send(response);
            }); 


            server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
                ESP.restart();
            });        


            //Display related URL handling
            // set up servers for displays            
            server.on("/disp0", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::Displays::instance()->renderDisplay(0));
            });

            server.on("/disp1", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::Displays::instance()->renderDisplay(1));
            });

            server.on("/disp2", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::Displays::instance()->renderDisplay(2));
            });

            server.on("/disp3", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(200, "text/plain", CANServer::Displays::instance()->renderDisplay(3));
            });

            //receive posts of display buttons, TODO do something with the buttons
            server.on("/post0", HTTP_POST, [](AsyncWebServerRequest * request) {
                request->send(200);
            });
            server.on("/post1", HTTP_POST, [](AsyncWebServerRequest * request) {
                request->send(200);
            });
            server.on("/post2", HTTP_POST, [](AsyncWebServerRequest * request) {
                request->send(200);
            });
            server.on("/post3", HTTP_POST, [](AsyncWebServerRequest * request) {
                request->send(200);
            });
            


            //Static content related URL handling
            server.on("/js/app.js", HTTP_GET,  [](AsyncWebServerRequest *request){
                AsyncWebServerResponse *response = request->beginResponse(CANServer::SPIFFileSystem::SPIFFS_app, "/js/app.js", "application/javascript");
                response->addHeader("Cache-Control", "max-age=600");
                request->send(response);
            });

            server.on("/css/app.css", HTTP_GET,  [](AsyncWebServerRequest *request){
                AsyncWebServerResponse *response = request->beginResponse(CANServer::SPIFFileSystem::SPIFFS_app, "/css/app.css", "text/css");
                response->addHeader("Cache-Control", "max-age=600");
                request->send(response);
            });



            //handle OTA updats
            //A command like this 
            //curl --progress-bar --verbose -F "file=@web-firmware.bin" http://172.20.21.204/update | cat
            //curl --progress-bar --verbose -F "file=@web-ui.bin" http://172.20.21.204/update_ui | cat
            //will send the update to the server

            server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
                // the request handler is triggered after the upload has finished... 
                // create the response, add header, and send response
                AsyncWebServerResponse *response = request->beginResponse(Update.hasError() ? 400 : 200);

                response->addHeader("Connection", "close");
                response->addHeader("Access-Control-Allow-Origin", "*");
                
                request->send(response);

            },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
                _handleUpdateChunk(U_FLASH, 1572864, request, filename, index, data, len, final);
            });

            server.on("/update_ui", HTTP_POST, [](AsyncWebServerRequest *request){
                // the request handler is triggered after the upload has finished... 
                // create the response, add header, and send response
                
                AsyncWebServerResponse *response = request->beginResponse(Update.hasError() ? 400 : 200);

                response->addHeader("Connection", "close");
                response->addHeader("Access-Control-Allow-Origin", "*");
                
                request->send(response);

            }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                _handleUpdateChunk(U_SPIFFS, 921600, request, filename, index, data, len, final);
            });

            // Start server
            server.begin();

            Serial.println("Done");
        }

        void _handleUpdateChunk(const int updateType, const unsigned long partitionSize, AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            //Upload handler chunks in data            
            if(index == 0) 
            { 
                // if index == 0 then this is the first frame of data
                Serial.printf("Update starting: %s\n", filename.c_str());
                Serial.setDebugOutput(true);

                PauseAllProcessing = true;
                
                // calculate sketch space required for the update
                if(!Update.begin(partitionSize, updateType))
                {
                    //start with max available size
                    Update.printError(Serial);
                }
            }

            //Write chunked data to the free sketch space
            if(Update.write(data, len) != len)
            {
                Update.printError(Serial);
            }
            
            if(final)
            { // if the final flag is set then this is the last frame of data
                if(Update.end(true))
                { 
                    //true to set the size to the current progress
                    Serial.printf("Update success: %u B\nRebooting...\n", index+len);
                    RebootAfterUpdate = true;
                } 
                else 
                {
                    Serial.println("Update failed");
                    Update.printError(Serial);
                }
                Serial.setDebugOutput(false);
                PauseAllProcessing = false;
            }
        }
    }
}
