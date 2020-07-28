#ifndef __DISPLAYSTATE_H__
#define __DISPLAYSTATE_H__

#include <Arduino.h>

namespace CANServer
{
    class DisplayState
    {
    public:
        static DisplayState *display0;
        static DisplayState *display1;
        static DisplayState *display2;
        static DisplayState *display3;

        ~DisplayState();

        void load();
        void save();

        const char* displayString() const;
        const uint displayStringLength() const;
        void updateDisplayString(const char* newValue);

        static inline const char* offDisplayString() { return "1m t0b1000r"; }

        static void loadAll();

    private:
        DisplayState(const int displayId);

        int _displayId;
        String _displayString;
    };
}

#endif