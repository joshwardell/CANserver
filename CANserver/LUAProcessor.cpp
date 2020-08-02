#include "LUAProcessor.h"

#include <SPIFFS.h>

#include "CanBus.h"

CANServer::LUAProcessor* CANServer::LUAProcessor::_instance = NULL;

extern "C" {
  static int lua_wrapper_print (lua_State *L) {
    int n = lua_gettop(L);  /* number of arguments */
    int i;
    lua_getglobal(L, "tostring");
    for (i=1; i<=n; i++) 
    {
      const char *s;
      size_t l;
      lua_pushvalue(L, -1);  /* function to be called */
      lua_pushvalue(L, i);   /* value to print */
      lua_call(L, 1, 1);
      s = lua_tolstring(L, -1, &l);  /* get result */
      if (s == NULL)
        return luaL_error(L, "'tostring' must return a string to 'print'");
      if (i>1) Serial.write("\t");
      Serial.write(s);
      lua_pop(L, 1);  /* pop result */
    }
    Serial.println();
    return 0;
  }

  static int lua_wrapper_getvar(lua_State *L) {
    int n = lua_gettop(L);
    if (n != 1)
    {
      //We expect a single argument - The variable name
      return luaL_error(L, "1 argument expected (<variable name>)");
    }
    else
    {
        if (lua_type(L, 1) == LUA_TSTRING)
        {
          const char* varName = lua_tostring(L, 1);

          //Check to see if this is a processed item first
          CANServer::LUAProcessor *luaprocessorInstance = CANServer::LUAProcessor::instance();
          CANServer::LUAProcessor::ProcessedItemMap::iterator item_it = luaprocessorInstance->processedItems()->find(varName);
          if (item_it != luaprocessorInstance->processedItems()->end())
          {
            lua_pushnumber(L, item_it->second);
          }
          else
          {
            CANServer::CanBus *canbusInstance = CANServer::CanBus::instance();
            CANServer::CanBus::AnalysisItemMap::const_iterator found_it = canbusInstance->dynamicAnalysisItems()->find(varName);
            if (found_it != canbusInstance->dynamicAnalysisItems()->end())
            {
              lua_pushnumber(L, found_it->second->lastValue);
            }
            else
            {
              //Check to see if this is a processed item
              lua_pushnumber(L, 0);
            }
          }
          return 1;
        }
        else
        {
          return luaL_error(L, "Argument expected to be a string");
        }
    }
  }

  static int lua_wrapper_setvar(lua_State *L) {
    int n = lua_gettop(L);
    if (n != 2)
    {
      //We expect a single argument - The variable name
      return luaL_error(L, "2 argument expected (<variable name>, <value>)");
    } 
    else
    {
      if (lua_type(L, 1) == LUA_TSTRING)
      {
        if (lua_type(L, 2) == LUA_TNUMBER)
        {
          const char* varName = lua_tostring(L, 1);
          CANServer::LUAProcessor* luaprocessorInstance = CANServer::LUAProcessor::instance();
          CANServer::LUAProcessor::ProcessedItemMap::iterator item_it = luaprocessorInstance->processedItems()->find(varName);
          if (item_it == luaprocessorInstance->processedItems()->end())
          {
            std::pair<CANServer::LUAProcessor::ProcessedItemMap::iterator, bool> returnPair = luaprocessorInstance->processedItems()->insert(CANServer::LUAProcessor::ProcessedItemPair(varName, 0));
            item_it = returnPair.first;
          }

          item_it->second = lua_tonumber(L, 2);
        }
        else
        {
          return luaL_error(L, "Argument 2 expected to be a number");
        }
      }
      else
      {
        return luaL_error(L, "Argument 1 expected to be a string");
      }
    }

    return 0;
  }
} 


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
  this->reloadScript();
}

const size_t _kBufferSize = 1024;
static uint8_t _s_buffer[_kBufferSize];
File _scriptFile;

const char* lua_SPIFFS_Loader( lua_State *L, void *data, size_t *size )
{
  memset(_s_buffer, 0, _kBufferSize);

  File *theFile = reinterpret_cast<File*>(data);
  if ( !theFile )
  {
    return NULL;
  }
  if (theFile->available() > 0)
  {
    size_t readCount = theFile->read(&_s_buffer[0], _kBufferSize);

    *size = readCount;

    return (const char*)_s_buffer;
  }
  else
  {
    *size = 0;
    return NULL;
  }
    
}

#define LUASCRIPT "/processing/script.lua"

void CANServer::LUAProcessor::saveNewScript(const char* scriptContents)
{
  File newScriptFile = SPIFFS.open(LUASCRIPT, "w");
  newScriptFile.print(scriptContents);
  newScriptFile.close();
}
void CANServer::LUAProcessor::reloadScript()
{
  Serial.println("Reloading LUA script...");
  //Switch out the lua_state so we aren't running it any more
  lua_State *currentState = _luaState;
  _luaState = NULL;

  //close out the old lua state
  if (currentState)
  {
    lua_close(currentState);
  }

  //Create a new lua state
  lua_State *newState = luaL_newstate();
  luaopen_base(newState);
  luaopen_table(newState);
  luaopen_string(newState);
  luaopen_math(newState);

  // delete some dangerous functions
  lua_pushnil(newState); lua_setglobal(newState, "dofile");
	lua_pushnil(newState); lua_setglobal(newState, "loadfile");
	lua_pushnil(newState); lua_setglobal(newState, "loadlib");
	lua_pushnil(newState); lua_setglobal(newState, "loadstring");
	lua_pushnil(newState); lua_setglobal(newState, "require");
	lua_pushnil(newState); lua_setglobal(newState, "rawequal");
	lua_pushnil(newState); lua_setglobal(newState, "rawget");
	lua_pushnil(newState); lua_setglobal(newState, "rawset");
	lua_pushnil(newState); lua_setglobal(newState, "getfenv");
	lua_pushnil(newState); lua_setglobal(newState, "setfenv");
	lua_pushnil(newState); lua_setglobal(newState, "newproxy");
	lua_pushnil(newState); lua_setglobal(newState, "gcinfo");
	lua_pushnil(newState); lua_setglobal(newState, "collectgarbage");

  //Register some handlers we provide
  lua_register(newState, "print", lua_wrapper_print);
  lua_register(newState, "CANServer_getVar", lua_wrapper_getvar);
  lua_register(newState, "CANServer_setVar", lua_wrapper_setvar);

  //Attempt to load the script from SPIFFS
  if (SPIFFS.exists(LUASCRIPT))
  {
    _scriptFile = SPIFFS.open(LUASCRIPT);
    if (_scriptFile)
    {
      int retCode = lua_load( newState, &lua_SPIFFS_Loader, &_scriptFile, NULL, NULL);
      if (retCode == 0)
      {
        Serial.println("LUA Load worked!");
        
        //Register the function as a global named function
        lua_setglobal(newState, "runProcessing");
        
        //Lua stack should be ready for running now
        _luaError = false;
        _luaState = newState;

        _scriptFile.close();
      }
      else
      {
        Serial.print("LUA Load failed: ");
        Serial.println(retCode);

        _luaError = true;

        //Pull the error off the stack and store it
        _luaErrorString = lua_tostring(newState, -1);
        lua_pop(newState, 1);
        
        lua_close(newState);
        newState = NULL;
      }

      return;
    }
  }

  Serial.println("No script file to load");
}


void CANServer::LUAProcessor::handle()
{
  unsigned long currentMillis = millis();
  if (currentMillis - _previousExecutionTime >= 50 && _luaState && !_luaError) 
  {
    unsigned long startTime = millis();
    //Find our method we want to run
    lua_getglobal(_luaState,"runProcessing");
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