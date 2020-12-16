#ifndef __DISPLAYS_H__
#define __DISPLAYS_H__

#include <Arduino.h>
#include "Average.h"

#include "lua/lua.hpp"

namespace CANServer
{
    class Displays
    {
    public:
        static Displays* instance();
        ~Displays();

        void setup();
        void loadScriptForDisplay(const uint8_t displayId);
        void saveScriptForDisplay(const uint8_t displayId, const char* script);

        const char* renderDisplay(const uint8_t displayId);

        const char* filenameForDisplay(const uint8_t displayId);
        const bool scriptErrorForDisplay(const uint8_t displayId) { return _luaError[displayId]; }
        const char* errorStringForDisplay(const uint8_t displayId) { return _luaErrorString[displayId].c_str(); }
        Average<uint16_t>* processingTimeForDisplay(const uint8_t displayId) { return &(_processingTime[displayId]); }

    private:

        Displays();

        lua_State *_luaState;

        bool _luaError[4];
        std::string _luaErrorString[4];

        std::string _lastDisplayStrings[4];

        Average<uint16_t> _processingTime[4];

        static Displays *_instance;
    };
}

#endif