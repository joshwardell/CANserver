#include "WebServer.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include <SPIFFS.h>

//The SPIFFS editor taks up a lot of flash space.  For development purposes it is nice to have, but for production it should prob be comented out
#define INCLUDE_SPIFFS_EDITOR
#ifdef INCLUDE_SPIFFS_EDITOR
#include <SPIFFSEditor.h>
#endif

#include <AsyncJson.h>
#include <SD.h>


#include "DisplayState.h"
#include "VehicleState.h"
#include "SDCard.h"
#include "CanBus.h"
#include "Network.h"
#include "Logging.h"

#include "ginger.h"

#define ASSIGN_HELPER(keyname) if (keyParam->value() == #keyname)\
                    {\
                        vehicleStateInstance->keyname = atoi(valParam->value().c_str());\
                    }
#define FETCH_HELPER(keyname) vehiclestatus[#keyname] = vehicleStateInstance->keyname;
AsyncWebServer server(80);

String _buildDate(const String& var)
{
  if(var == "BUILD_DATE")
    return F(__DATE__ " " __TIME__);

  return String();
}

namespace CANServer
{
    namespace WebServer
    {
        void _populateTemplateVaraibles(ginger::temple *vars);

        void _renderDisplay(AsyncWebServerRequest *request, DisplayState* display)
        {
            if (CANServer::VehicleState::instance()->DisplayOn)
            {
                ginger::temple t;
                CANServer::WebServer::_populateTemplateVaraibles(&t);
                
                try {
                    std::stringstream ss;
                    ginger::parse(display->displayString(), t, ginger::from_ios(ss));

                    request->send(200, "text/plain", ss.str().c_str());

                } catch (ginger::parse_error& error) {
                    //Serial.println(error.long_error().c_str());
                    request->send(200, "text/plain", "1m2s DISPLAY  ERROR  t500r");
                }
            }
            else
            {
                request->send(200, "text/plain", CANServer::DisplayState::offDisplayString());
            }
        }

        void setup()
        {
            Serial.println("Setting up Web Server...");

        #ifdef INCLUDE_SPIFFS_EDITOR
            //Attach the SPIFFS editor helper so we can edit files on the fly
            server.addHandler(new SPIFFSEditor(SPIFFS, "admin","password"));
        #endif

            server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
                //TODO FIRMWARE DATE
                request->send(SPIFFS, "/html/index.html", String(), false, _buildDate);
            });





            //Display configuration related url handling
            server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/html/config.html");
            });

            server.on("/config_save", HTTP_POST, [](AsyncWebServerRequest * request) {
                
                if(request->hasParam("disp0", true))
                {
                    AsyncWebParameter* newValue = request->getParam("disp0", true);
                    CANServer::DisplayState::display0->updateDisplayString(newValue->value().c_str());
                    CANServer::DisplayState::display0->save();
                }

                if(request->hasParam("disp1", true))
                {
                    AsyncWebParameter* newValue = request->getParam("disp1", true);
                    CANServer::DisplayState::display1->updateDisplayString(newValue->value().c_str());
                    CANServer::DisplayState::display1->save();
                }

                if(request->hasParam("disp2", true))
                {
                    AsyncWebParameter* newValue = request->getParam("disp2", true);
                    CANServer::DisplayState::display2->updateDisplayString(newValue->value().c_str());
                    CANServer::DisplayState::display2->save();
                }

                if(request->hasParam("disp3", true))
                {
                    AsyncWebParameter* newValue = request->getParam("disp3", true);
                    CANServer::DisplayState::display3->updateDisplayString(newValue->value().c_str());
                    CANServer::DisplayState::display3->save();
                }

                request->redirect("/config");
            });

            server.on("/config_update", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                JsonObject displaysettings = doc.createNestedObject("displaysettings");
                
                displaysettings["disp0"] = CANServer::DisplayState::display0->displayString();
                displaysettings["disp1"] = CANServer::DisplayState::display1->displayString();
                displaysettings["disp2"] = CANServer::DisplayState::display2->displayString();
                displaysettings["disp3"] = CANServer::DisplayState::display3->displayString();
                
                response->setLength();
                request->send(response);
            }); 

            //Network configuration related url handling
            server.on("/network", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/html/network.html");
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

           




            //Debug related url handling
            server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send(SPIFFS, "/html/debug.html");
            });

            server.on("/debug_save", HTTP_POST, [](AsyncWebServerRequest * request) {
                //Handle any incomming post vars and update the vehicle state as required

                if(request->hasParam("key", true) && request->hasParam("value", true))
                {
                    AsyncWebParameter* keyParam = request->getParam("key", true);
                    AsyncWebParameter* valParam = request->getParam("value", true);

                    CANServer::VehicleState *vehicleStateInstance = CANServer::VehicleState::instance();
                    ASSIGN_HELPER(BattVolts)
                    ASSIGN_HELPER(BattAmps)

                    if (keyParam->value() == "BattVolts")
                    {
                        vehicleStateInstance->BattVolts = atoi(valParam->value().c_str());

                        vehicleStateInstance->BattPower = vehicleStateInstance->BattVolts * vehicleStateInstance->BattAmps / 100;
                    }

                    if (keyParam->value() == "BattAmps")
                    {
                        vehicleStateInstance->BattAmps = atoi(valParam->value().c_str());

                        vehicleStateInstance->BattPower = vehicleStateInstance->BattVolts * vehicleStateInstance->BattAmps / 100;
                    }
                    
                    if (keyParam->value() == "RearTorque")
                    {
                        vehicleStateInstance->RearTorque = atoi(valParam->value().c_str());
                    }

                    ASSIGN_HELPER(FrontTorque)
                    ASSIGN_HELPER(MinBattTemp)
                    ASSIGN_HELPER(BattCoolantRate)
                    ASSIGN_HELPER(PTCoolantRate)
                    ASSIGN_HELPER(MaxRegen)
                    ASSIGN_HELPER(MaxDisChg)
                    ASSIGN_HELPER(VehSpeed)
                    ASSIGN_HELPER(SpeedUnit)
                    ASSIGN_HELPER(v12v261)
                    ASSIGN_HELPER(BattCoolantTemp)
                    ASSIGN_HELPER(PTCoolantTemp)
                    ASSIGN_HELPER(BattRemainKWh)
                    ASSIGN_HELPER(BattFullKWh)
                    ASSIGN_HELPER(InvHStemp376)
                    ASSIGN_HELPER(BSR)
                    ASSIGN_HELPER(BSL)
                    ASSIGN_HELPER(DisplayOn)
                    
                    request->send(200);
                }
                else
                {
                    request->send(404);
                }
            });

            server.on("/debug_update", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                JsonObject vehiclestatus = doc.createNestedObject("vehiclestatus");

                CANServer::VehicleState *vehicleStateInstance = CANServer::VehicleState::instance();
                FETCH_HELPER(BattVolts)
                FETCH_HELPER(BattAmps)
                FETCH_HELPER(RearTorque)
                FETCH_HELPER(FrontTorque)
                FETCH_HELPER(MinBattTemp)
                FETCH_HELPER(BattCoolantRate)
                FETCH_HELPER(PTCoolantRate)
                FETCH_HELPER(MaxRegen)
                FETCH_HELPER(MaxDisChg)
                FETCH_HELPER(VehSpeed)
                FETCH_HELPER(SpeedUnit)
                FETCH_HELPER(v12v261)
                FETCH_HELPER(BattCoolantTemp)
                FETCH_HELPER(PTCoolantTemp)
                FETCH_HELPER(BattRemainKWh)
                FETCH_HELPER(BattFullKWh)
                FETCH_HELPER(InvHStemp376)
                FETCH_HELPER(BSR)
                FETCH_HELPER(BSL)
                FETCH_HELPER(DisplayOn)

                JsonObject dynamicanalysisitems = doc.createNestedObject("dynamicanalysisitems");
                for (CANServer::CanBus::AnalysisItemMap::const_iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->begin(); it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end(); it++)
                {
                    CANServer::CanBus::AnalysisItem *analysisItem = it->second;
                    dynamicanalysisitems[it->first] = analysisItem->_lastValue;
                }
                
                response->setLength();
                request->send(response);
            });    






            //Accessability to log files
            server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
                request->send(SPIFFS, "/html/logs.html");
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
                LOGSettingsUpdateHelper("seriallog", CANServer::Logging::LogType_Serial);
                
                JsonObject sdDetailsNode = doc.createNestedObject("sddetails");
                sdDetailsNode["available"] = SDCard::available();
                if (SDCard::available())
                {
                    sdDetailsNode["totalkbytes"] = (uint32_t)(SD.totalBytes() / (1024 * 1024));
                    sdDetailsNode["usedkbytes"] = (uint32_t)(SD.usedBytes() / (1024 * 1024));
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
                LOGSettingsSaveHelper("seriallog", CANServer::Logging::LogType_Serial);

                CANServer::Logging::instance()->saveConfiguraiton();

                request->redirect("/logs");
            });    




            //Dynamic Analysis configuration
            server.on("/analysis", HTTP_GET, [](AsyncWebServerRequest *request){
                request->send(SPIFFS, "/html/analysis.html");
            });

            server.on("/analysis_delete", HTTP_POST, [](AsyncWebServerRequest * request) {
                if(request->hasParam("name", true))
                {
                    std::string itemName = request->getParam("name", true)->value().c_str();

                    CANServer::CanBus::instance()->pauseDynamicAnalysis();
                   
                    CANServer::CanBus::AnalysisItemMap::iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->find(itemName);
                    if (it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end())
                    {
                        delete it->second;
                        CANServer::CanBus::instance()->dynamicAnalysisItems()->erase(itemName);
                    }                        
                    
                    CANServer::CanBus::instance()->saveDynamicAnalysisConfiguration();
                    CANServer::CanBus::instance()->resumeDynamicAnalysis();

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
                    request->hasParam("name", true) &&
                    request->hasParam("frameid", true) &&
                    request->hasParam("startbit", true) &&
                    request->hasParam("bitlength", true) &&
                    request->hasParam("factor", true) &&
                    request->hasParam("signaloffset", true))
                {
                    std::string itemName = request->getParam("name", true)->value().c_str();

                    CANServer::CanBus::AnalysisItem *analysisItem = new CANServer::CanBus::AnalysisItem();
                    analysisItem->_frameId = atoi(request->getParam("frameid", true)->value().c_str());
                    analysisItem->_startBit = atoi(request->getParam("startbit", true)->value().c_str());
                    analysisItem->_bitLength = atoi(request->getParam("bitlength", true)->value().c_str());
                    analysisItem->_factor = atof(request->getParam("factor", true)->value().c_str());
                    analysisItem->_signalOffset = atoi(request->getParam("signaloffset", true)->value().c_str());

                    if (request->hasParam("issigned", true) && request->getParam("issigned", true)->value() == "true")
                    {
                        analysisItem->_isSigned = true;
                    }
                    else
                    {
                        analysisItem->_isSigned = false;
                    }

                    if (request->hasParam("littleendian", true) && request->getParam("littleendian", true)->value() == "true")
                    {
                        analysisItem->_byteOrder = true;
                    }
                    else
                    {
                        analysisItem->_byteOrder = true;
                    }


                    CANServer::CanBus::instance()->pauseDynamicAnalysis();
                    if(request->hasParam("state", true) && request->getParam("state", true)->value() == "update")
                    {
                        //Clean up the old entry so we can replace it
                        CANServer::CanBus::AnalysisItemMap::iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->find(itemName);
                        if (it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end())
                        {
                            delete it->second;
                            CANServer::CanBus::instance()->dynamicAnalysisItems()->erase(itemName);
                        }                        
                    }

                    //insert our new entry
                    CANServer::CanBus::instance()->dynamicAnalysisItems()->insert(CANServer::CanBus::AnalysisItemPair(itemName, analysisItem));

                    CANServer::CanBus::instance()->saveDynamicAnalysisConfiguration();
                    CANServer::CanBus::instance()->resumeDynamicAnalysis();
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
                    JsonObject dynamicanalysisitem = doc.createNestedObject(it->first);
                    CANServer::CanBus::AnalysisItem *analysisItem = it->second;

                    dynamicanalysisitem["frameid"] = analysisItem->_frameId;
                    dynamicanalysisitem["startBit"] = analysisItem->_startBit;
                    dynamicanalysisitem["bitLength"] = analysisItem->_bitLength;
                    dynamicanalysisitem["factor"] = analysisItem->_factor;
                    dynamicanalysisitem["signalOffset"] = analysisItem->_signalOffset;
                    dynamicanalysisitem["isSigned"] = analysisItem->_isSigned;
                    dynamicanalysisitem["byteOrder"] = analysisItem->_byteOrder;
                }

                response->setLength();
                request->send(response);
            }); 

            server.on("/analysis_update", HTTP_GET, [](AsyncWebServerRequest *request) {
                AsyncJsonResponse * response = new AsyncJsonResponse();
                JsonVariant& doc = response->getRoot();

                for (CANServer::CanBus::AnalysisItemMap::const_iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->begin(); it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end(); it++)
                {
                    CANServer::CanBus::AnalysisItem *analysisItem = it->second;
                    doc[it->first] = analysisItem->_lastValue;
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
                _renderDisplay(request, CANServer::DisplayState::display0);
            });

            server.on("/disp1", HTTP_GET, [](AsyncWebServerRequest *request){
                _renderDisplay(request, CANServer::DisplayState::display1);
            });

            server.on("/disp2", HTTP_GET, [](AsyncWebServerRequest *request){
                _renderDisplay(request, CANServer::DisplayState::display2);
            });

            server.on("/disp3", HTTP_GET, [](AsyncWebServerRequest *request){
                _renderDisplay(request, CANServer::DisplayState::display3);
            });

            //receive posts of display buttons, TODO do something with the buttons
            server.on("/post0", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                request->send(200);
            });
            server.on("/post1", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                request->send(200);
            });
            server.on("/post2", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                request->send(200);
            });
            server.on("/post3", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL,
                    [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
                request->send(200);
            });
            


            //Static content related URL handling
            server.on("/js/zepto.min.js", HTTP_GET,  [](AsyncWebServerRequest *request){
                AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/js/zepto.min.js", "application/javascript");
                response->addHeader("Cache-Control", "max-age=600");
                request->send(response);
            });



            // Start server
            server.begin();

            Serial.println("Done");
        }

#define POPULATEHELPER(name, multiplier) {\
(*vars)[#name] = vehicleStateInstance->name * multiplier;\
}
        void _populateTemplateVaraibles(ginger::temple *vars)
        {
            CANServer::VehicleState *vehicleStateInstance = CANServer::VehicleState::instance();

            //Deal with our statically analysised items
            POPULATEHELPER(BattPower, 1);
            POPULATEHELPER(BattVolts, 10)
            POPULATEHELPER(BattAmps, 10)
            POPULATEHELPER(RearTorque, 10)
            POPULATEHELPER(FrontTorque, 10)
            POPULATEHELPER(MinBattTemp, 10)
            POPULATEHELPER(BattCoolantRate, 10)
            POPULATEHELPER(PTCoolantRate, 10)
            POPULATEHELPER(MaxRegen, 10)
            POPULATEHELPER(MaxDisChg, 10)
            
            if (vehicleStateInstance->SpeedUnit == 1) 
            { 
                //MPH
                (*vars)["VehSpeed"] = int(0.621371 * vehicleStateInstance->VehSpeed * 10);
            } 
            else 
            {
                //KMH
                (*vars)["VehSpeed"] = vehicleStateInstance->VehSpeed * 10;
            }

            if (vehicleStateInstance->SpeedUnit == 1) 
            { 
                //MPH
                (*vars)["SpeedUnitString"] = "HPM";
            } 
            else 
            {
                //KPH
                (*vars)["SpeedUnitString"] = "HPK";
            }

            POPULATEHELPER(SpeedUnit, 1)
            POPULATEHELPER(v12v261, 10)
            POPULATEHELPER(BattCoolantTemp, 10)
            POPULATEHELPER(PTCoolantTemp, 10)
            POPULATEHELPER(BattRemainKWh, 10)
            POPULATEHELPER(BattFullKWh, 10)
            POPULATEHELPER(InvHStemp376, 10)
            POPULATEHELPER(BSR, 1)
            POPULATEHELPER(BSL, 1)
            POPULATEHELPER(DisplayOn, 1)

            //Some scaled vars that are used by the default bargraph display
            (*vars)["BattPower_Scaled_Bar"] = int(0.008 * vehicleStateInstance->BattPower);
            (*vars)["RearTorque_Scaled_Bar"] = int(0.006 * vehicleStateInstance->RearTorque);


            //Deal with our dynamically analysised items
            for (CANServer::CanBus::AnalysisItemMap::const_iterator it = CANServer::CanBus::instance()->dynamicAnalysisItems()->begin(); it != CANServer::CanBus::instance()->dynamicAnalysisItems()->end(); it++)
            {
                CANServer::CanBus::AnalysisItem *analysisItem = it->second;
                (*vars)[it->first] = analysisItem->_lastValue * 10;
            }
        }
    }
}