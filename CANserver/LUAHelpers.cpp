#include "LUAHelpers.h"

#include "CanBus.h"



extern "C" {
    static int lua_wrapper_print (lua_State *L);
    static int lua_wrapper_getvar(lua_State *L);
}


CANServer::LUAHelpers::SPIFFSLoader::SPIFFSLoader()
{

}
            
const CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult CANServer::LUAHelpers::SPIFFSLoader::load(const char* filename, lua_State *L, const char* functionName, std::string* errorString)
{
    ScriptLoadResult returnCode = CANServer::LUAHelpers::SPIFFSLoader::ScriptLoadResult_OK;
    if (SPIFFS.exists(filename))
    {
        //Serial.println("File exists");
        _fileToLoad = SPIFFS.open(filename);
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
                    return 1;
                }
                else
                {
                    return luaL_error(L, "Argument expected to be a string");
                }
        }
    }
}