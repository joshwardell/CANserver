#include "LUAProcessor.h"

#include "LUAHelpers.h"

CANServer::LUAProcessor* CANServer::LUAProcessor::_instance = NULL;


CANServer::LUAProcessor::LUAProcessor() : _processingTime(20)
{
    _previousExecutionTime = 0;

    _luaState = NULL; 
    _luaError = false;
}


CANServer::LUAProcessor::~LUAProcessor()
{

}

CANServer::LUAProcessor* CANServer::LUAProcessor::instance()
{
    if (_instance == NULL)
    {
        _instance = new CANServer::LUAProcessor();
    }

    return _instance;
}

void CANServer::LUAProcessor::setup()
{
    this->loadScript();
}

#define LUASCRIPT "/processing/script.lua"
#define FUNCTIONNAME "runProcessing"

void CANServer::LUAProcessor::saveNewScript(const char* scriptContents)
{
    File newScriptFile = SPIFFS.open(LUASCRIPT, "w");
    newScriptFile.print(scriptContents);
    newScriptFile.close();
}
void CANServer::LUAProcessor::loadScript()
{
    Serial.println("Loading processing script...");
    //Switch out the lua_state so we aren't running it any more
    lua_State *currentState = _luaState;
    _luaState = NULL;

    if (currentState == NULL)
    {
        currentState = luaL_newstate();
        CANServer::LUAHelpers::setupLUAState(currentState, true);
    }

    CANServer::LUAHelpers::SPIFFSLoader scriptLoader;
    CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult loadResult = scriptLoader.load(LUASCRIPT, currentState, FUNCTIONNAME, &_luaErrorString);
    if (loadResult != CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_OK)
    {
        Serial.println("Script is bad");
        _luaError = true;
    }
    else
    {
        //There was an error loading the LUA.  Error string has already been set
        Serial.println("Script is good");
        _luaError = false;
    }

    _luaState = currentState;
}


void CANServer::LUAProcessor::handle()
{
    unsigned long currentMillis = millis();
    if (currentMillis - _previousExecutionTime >= 50 && _luaState && !_luaError) 
    {
        unsigned long startTime = millis();
        //Find our method we want to run
        lua_getglobal(_luaState,FUNCTIONNAME);
        if(lua_isfunction(_luaState, -1) )
        {
            //Run it
            if (lua_pcall(_luaState,0,0,0) == 0)
            {
                //Call worked.  We should have a normalized stack at this point.

                //Clear out our error state
                _luaError = false;
                if (_luaErrorString.length() > 0)
                {
                    _luaErrorString = "";
                }
            }
            else
            {
                //We got an error running the Lua code.  Don't keep executing
                _luaError = true;

                //Pull the error off the stack and store it
                _luaErrorString = lua_tostring(_luaState, -1);
                lua_pop(_luaState, 1);
            }
        }
        unsigned long endTime = millis();

        _processingTime.push(endTime - startTime);

        _previousExecutionTime = currentMillis;
    }
}