#include "LUAHelpers.h"

#include "CanBus.h"
#include "SPIFFileSystem.h"


//Container for user defined variables
typedef std::map<std::string, float> UserDefinedVarMap;
typedef std::pair<std::string, float> UserDefinedVarPair;
UserDefinedVarMap _userDefinedVars;


extern "C" {
    static int lua_wrapper_print (lua_State *L);
    static int lua_wrapper_getvar(lua_State *L);
    static int lua_wrapper_setvar(lua_State *L);
    static int lua_wrapper_clearvar(lua_State *L);
    static int lua_wrapper_getanalysisvar(lua_State *L);
}


CANServer::LUAHelpers::SPIFFSLoader::SPIFFSLoader()
{

}
            
const CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult CANServer::LUAHelpers::SPIFFSLoader::load(const char* filename, lua_State *L, const char* functionName, std::string* errorString)
{
    ScriptLoadResult returnCode = CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_OK;
    
    if (CANServer::SPIFFileSystem::SPIFFS_data.exists(filename))
    {
        //Serial.println("File exists");
        _fileToLoad = CANServer::SPIFFileSystem::SPIFFS_data.open(filename);
        if (_fileToLoad)
        {
            //Serial.println("File opened");
            //ensure that the funciton we are loading doesn't exist
            lua_pushnil(L); lua_setglobal(L, functionName);

            //Serial.println("function nil'ed");

            int retCode = lua_load( L, &CANServer::LUAHelpers::SPIFFSLoader::lua_SPIFFS_Loader, this, NULL, NULL);
            if (retCode == 0)
            {
                //Serial.println("LUA Load worked!");

                //Register the function as a global named function
                lua_setglobal(L, functionName);

                returnCode = CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_OK;
                *errorString = "";
            }
            else
            {
                //Serial.print("LUA Load failed: ");
                //Serial.println(retCode);

                returnCode = CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_LUALoadFailed;

                //Pull the error off the stack and store it
                *errorString = lua_tostring(L, -1);
                lua_pop(L, 1);
            }

            _fileToLoad.close();
        }
        else
        {
            //File could not be opened
            *errorString = "File could not be opened";
            returnCode = CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_FileOpenFailed;
        }
    }
    else
    {
        //File not found
        *errorString = "File not found";
        returnCode = CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_FileNotFound;
    }

    return returnCode;
}


const char* CANServer::LUAHelpers::SPIFFSLoader::lua_SPIFFS_Loader( lua_State *L, void *data, size_t *size )
{
    //Serial.println("lua_SPIFFS_Loader called");
    CANServer::LUAHelpers::SPIFFSLoader *theThis = reinterpret_cast<CANServer::LUAHelpers::SPIFFSLoader*>(data);
    memset(theThis->_s_buffer, 0, LOAD_BUFFER_SIZE);

    if ( !theThis->_fileToLoad )
    {
        return NULL;
    }

    if (theThis->_fileToLoad.available() > 0)
    {
        //Serial.println("theThis->_fileToLoad.available() > 0");
        size_t readCount = theThis->_fileToLoad.read(&theThis->_s_buffer[0], LOAD_BUFFER_SIZE);

        *size = readCount;

        return (const char*)theThis->_s_buffer;
    }
    else
    {
        *size = 0;
        return NULL;
    }
}


void CANServer::LUAHelpers::setupLUAState(lua_State *L)
{
    luaopen_base(L);
    luaopen_table(L);
    luaopen_string(L);
    luaopen_math(L);

    // delete some dangerous functions
    lua_pushnil(L); lua_setglobal(L, "dofile");
	lua_pushnil(L); lua_setglobal(L, "loadfile");
	lua_pushnil(L); lua_setglobal(L, "loadlib");
	lua_pushnil(L); lua_setglobal(L, "loadstring");
	lua_pushnil(L); lua_setglobal(L, "require");
	lua_pushnil(L); lua_setglobal(L, "rawequal");
	lua_pushnil(L); lua_setglobal(L, "rawget");
	lua_pushnil(L); lua_setglobal(L, "rawset");
	lua_pushnil(L); lua_setglobal(L, "getfenv");
	lua_pushnil(L); lua_setglobal(L, "setfenv");
	lua_pushnil(L); lua_setglobal(L, "newproxy");
	lua_pushnil(L); lua_setglobal(L, "gcinfo");
	lua_pushnil(L); lua_setglobal(L, "collectgarbage");

    //Register some handlers we provide
    lua_register(L, "print", lua_wrapper_print);
    lua_register(L, "CANServer_getVar", lua_wrapper_getvar);
    lua_register(L, "CANServer_setVar", lua_wrapper_setvar);
    lua_register(L, "CANServer_clearVar", lua_wrapper_clearvar);
    lua_register(L, "CANServer_getAnalysisVar", lua_wrapper_getanalysisvar);
}




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

    static int lua_wrapper_clearvar(lua_State *L) {
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
                UserDefinedVarMap::iterator item_it = _userDefinedVars.find(varName);
                if (item_it != _userDefinedVars.end())
                {
                    _userDefinedVars.erase(item_it);
                }
                return 0;
            }
            else
            {
                return luaL_error(L, "Argument expected to be a string");
            }
        }
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

                UserDefinedVarMap::iterator item_it = _userDefinedVars.find(varName);
                if (item_it != _userDefinedVars.end())
                {
                    lua_pushnumber(L, item_it->second);
                }
                else
                {
                    //Nothing found.  Just return a 0
                    lua_pushnumber(L, 0);
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

                    UserDefinedVarMap::iterator item_it = _userDefinedVars.find(varName);
                    if (item_it == _userDefinedVars.end())
                    {
                        std::pair<UserDefinedVarMap::iterator, bool> returnPair = _userDefinedVars.insert(UserDefinedVarPair(varName, 0));
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

    static int lua_wrapper_getanalysisvar(lua_State *L) {
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

                    CANServer::CanBus *canbusInstance = CANServer::CanBus::instance();
                    CANServer::CanBus::AnalysisItemMap::const_iterator found_it = canbusInstance->dynamicAnalysisItems()->find(varName);
                    if (found_it != canbusInstance->dynamicAnalysisItems()->end())
                    {
                        lua_pushnumber(L, found_it->second->lastValue);
                    }
                    else
                    {
                        //Nothing found.  Just return a 0
                        lua_pushnumber(L, 0);
                    }
                    return 1;
                }
                else
                {
                    return luaL_error(L, "Argument expected to be a string");
                }
        }
    }
}