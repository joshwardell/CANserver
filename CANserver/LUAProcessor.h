#ifndef MY_LUA_WRAPPER_H
#define MY_LUA_WRAPPER_H

#include <Arduino.h>
#include <Average.h>
#include <map>


#define LUA_USE_C89
#include "lua/lua.hpp"

namespace CANServer 
{
    class LUAProcessor{
    public:
        static LUAProcessor* instance();
        ~LUAProcessor();

        void setup();
        void handle();

        void reloadScript();

        void saveNewScript(const char* scriptContents);

        const bool scriptError() const { return _luaError; };
        const char* errorString() const { return _luaErrorString.c_str(); };

        Average<uint16_t>* processingTime() { return &_processingTime; }

        typedef std::map<std::string, float> ProcessedItemMap;
        typedef std::pair<std::string, float> ProcessedItemPair;

        ProcessedItemMap* processedItems() { return &_processedItems; };
    private:
        LUAProcessor();

        lua_State *_luaState;
        bool _luaError;
        std::string _luaErrorString;

        unsigned long _previousExecutionTime;

        Average<uint16_t> _processingTime;

        ProcessedItemMap _processedItems;

        static LUAProcessor *_instance;
    };
}

#endif
