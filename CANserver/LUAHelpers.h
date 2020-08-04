#ifndef __LUAHELPERS_H__
#define __LUAHELPERS_H__

#include <Arduino.h>
#include <SPIFFS.h>

#include "lua/lua.hpp"

#define LOAD_BUFFER_SIZE 1024

namespace CANServer
{
    namespace LUAHelpers
    {
        void setupLUAState(lua_State *L);

        class SPIFFSLoader
        {
        public:
            static const char* lua_SPIFFS_Loader( lua_State *L, void *data, size_t *size );

            SPIFFSLoader();

            typedef enum {
                ScriptLoadResult_OK = 0,
                ScriptLoadResult_FileNotFound = 1,
                ScriptLoadResult_FileOpenFailed = 2,
                ScriptLoadResult_LUALoadFailed = 3
            } ScriptLoadResult;

            const ScriptLoadResult load(const char* filename, lua_State *L, const char* functionName, std::string* errorString);
        private:

            uint8_t _s_buffer[LOAD_BUFFER_SIZE];

            File _fileToLoad;
        };
    }
}
#endif