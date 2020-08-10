#include "Displays.h"

#include "LUAHelpers.h"
#include "CanBus.h"

#define DISPLAY0_FILENAME "/displays/0.lua"
#define DISPLAY0_FUNCTIONNAME "runDisplay0"
#define DISPLAY1_FILENAME "/displays/1.lua"
#define DISPLAY1_FUNCTIONNAME "runDisplay1"
#define DISPLAY2_FILENAME "/displays/2.lua"
#define DISPLAY2_FUNCTIONNAME "runDisplay2"
#define DISPLAY3_FILENAME "/displays/3.lua"
#define DISPLAY3_FUNCTIONNAME "runDisplay3"

CANServer::Displays* CANServer::Displays::_instance = NULL;

CANServer::Displays::Displays(): _processingTime{20, 20, 20, 20}
{
  _luaState = NULL; 

  _luaError[0] = false;
  _luaError[1] = false;
  _luaError[2] = false;
  _luaError[3] = false;

}

CANServer::Displays::~Displays()
{

}

CANServer::Displays* CANServer::Displays::instance()
{
  if (_instance == NULL)
  {
    _instance = new CANServer::Displays();
  }

  return _instance;
}

const char* CANServer::Displays::filenameForDisplay(const uint8_t displayId)
{
    //We only support up to 4 displays
    if (displayId > 3 || displayId < 0)
    {
        Serial.print("Request to load invalid display: ");
        Serial.println(displayId);
        return "";
    }

    switch (displayId)
    {
        case 0:
        {
            return DISPLAY0_FILENAME;
            break;
        }
        case 1:
        {
            return DISPLAY1_FILENAME;
            break;
        }
        case 2:
        {
            return DISPLAY2_FILENAME;
            break;
        }
        case 3:
        {
            return DISPLAY3_FILENAME;
            break;
        }
    }

    return "";
}

void CANServer::Displays::setup()
{
    Serial.println("Setting up Display Scripting...");
    if (_luaState == NULL)
    {
        _luaState = luaL_newstate();
        CANServer::LUAHelpers::setupLUAState(_luaState);
    }

    this->loadScriptForDisplay(0);
    this->loadScriptForDisplay(1);
    this->loadScriptForDisplay(2);
    this->loadScriptForDisplay(3);
    
    Serial.println("Done");
}

void CANServer::Displays::loadScriptForDisplay(const uint8_t displayId)
{
    //We only support up to 4 displays
    if (displayId > 3 || displayId < 0)
    {
        Serial.print("Request to load invalid display: ");
        Serial.println(displayId);
        return;
    }

    Serial.print("Loading display ");
    Serial.print(displayId);
    Serial.println(" script...");

    CANServer::LUAHelpers::SPIFFSLoader scriptLoader;

    const char* fileName = NULL;
    const char* functionName = NULL;

    bool *luaError = NULL;
    std::string* luaErrorString = NULL;

    switch (displayId)
    {
        case 0:
        {
            fileName = DISPLAY0_FILENAME;
            functionName = DISPLAY0_FUNCTIONNAME;

            luaError = &(_luaError[0]);
            luaErrorString = &(_luaErrorString[0]);

            break;
        }
        case 1:
        {
            fileName = DISPLAY1_FILENAME;
            functionName = DISPLAY1_FUNCTIONNAME;

            luaError = &(_luaError[1]);
            luaErrorString = &(_luaErrorString[1]);

            break;
        }
        case 2:
        {
            fileName = DISPLAY2_FILENAME;
            functionName = DISPLAY2_FUNCTIONNAME;

            luaError = &(_luaError[2]);
            luaErrorString = &(_luaErrorString[2]);

            break;
        }
        case 3:
        {
            fileName = DISPLAY3_FILENAME;
            functionName = DISPLAY3_FUNCTIONNAME;

            luaError = &(_luaError[3]);
            luaErrorString = &(_luaErrorString[3]);

            break;
        }
    }

    CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult loadResult = scriptLoader.load(fileName, _luaState, functionName, luaErrorString);
    if (loadResult != CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_OK)
    {
        //There was an error loading the LUA.  Error string has already been set
        *luaError = true;
    }
    else
    {
        *luaError = false;
    }
}

void CANServer::Displays::saveScriptForDisplay(const uint8_t displayId, const char* scriptContents)
{
    //We only support up to 4 displays
    if (displayId > 3 || displayId < 0)
    {
        Serial.print("Request to save invalid display: ");
        Serial.println(displayId);
        return;
    }

    Serial.print("Saving display ");
    Serial.print(displayId);
    Serial.println(" script...");

    const char* fileName = NULL;
    
    switch (displayId)
    {
        case 0:
        {
            fileName = DISPLAY0_FILENAME;
            break;
        }
        case 1:
        {
            fileName = DISPLAY1_FILENAME;
            break;
        }
        case 2:
        {
            fileName = DISPLAY2_FILENAME;
            break;
        }
        case 3:
        {
            fileName = DISPLAY3_FILENAME;
            break;
        }
    }

    File newScriptFile = SPIFFS.open(fileName, "w");
    newScriptFile.print(scriptContents);
    newScriptFile.close();
}

const char* CANServer::Displays::renderDisplay(const uint8_t displayId)
{
    //We only support up to 4 displays
    if (displayId > 3 || displayId < 0)
    {
        return "";
    }

    std::string *stringToReturn = NULL;
    const char* functionName = NULL;

    bool *luaError = NULL;
    std::string* luaErrorString = NULL;

    Average<uint16_t>* processingTime = NULL;

    switch(displayId)
    {
        case 0:
        {
            functionName = DISPLAY0_FUNCTIONNAME;
            stringToReturn = &_lastDisplayStrings[0];

            luaError = &(_luaError[0]);
            luaErrorString = &(_luaErrorString[0]);

            processingTime = &_processingTime[0];
            break;
        }
        case 1:
        {
            functionName = DISPLAY1_FUNCTIONNAME;
            stringToReturn = &_lastDisplayStrings[1];

            luaError = &(_luaError[1]);
            luaErrorString = &(_luaErrorString[1]);

            processingTime = &_processingTime[1];
            break;
        }
        case 2:
        {
            functionName = DISPLAY2_FUNCTIONNAME;
            stringToReturn = &_lastDisplayStrings[2];

            luaError = &(_luaError[2]);
            luaErrorString = &(_luaErrorString[2]);

            processingTime = &_processingTime[2];
            break;
        }
        case 3:
        {
            functionName = DISPLAY3_FUNCTIONNAME;
            stringToReturn = &_lastDisplayStrings[3];

            luaError = &(_luaError[3]);
            luaErrorString = &(_luaErrorString[3]);

            processingTime = &_processingTime[3];
            break;
        }
    }

    unsigned long startTime = millis();
    //Find our method we want to run
    lua_getglobal(_luaState, functionName);
    if(lua_isfunction(_luaState, -1) )
    {
        //Run it
        if (lua_pcall(_luaState,0,1,0) == 0)
        {
            //Call worked.  We should have a normalized stack at this point.

            //We should have the string we want on the stack.  Get it and pop it
            if(lua_isstring(_luaState, -1) )
            {
                *stringToReturn = lua_tostring(_luaState, -1);
                lua_pop(_luaState, 1);

                //Clear out our error state
                *luaError = false;
                if (luaErrorString->length() > 0)
                {
                    *luaErrorString = "";
                }
            }
            else
            {
                //We got a return value we didn't expect.  Just pop it so we don't leak
                lua_pop(_luaState, 1);

                *luaError = true;
                *luaErrorString = "Unexpected return value.  Expecting <string>";
            }
        }
        else
        {
            //We got an error running the Lua code.  Don't keep executing
            *luaError = true;

            //Pull the error off the stack and store it
            *luaErrorString = lua_tostring(_luaState, -1);
            lua_pop(_luaState, 1);
        }

        lua_gc (_luaState, LUA_GCCOLLECT, 0);
    }
    unsigned long endTime = millis();

    processingTime->push(endTime - startTime);

    if (*luaError)
    {
        *stringToReturn = "1m2s DISPLAY  ERROR  t500r";
    }

    return stringToReturn->c_str();
}