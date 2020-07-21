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

        ~DisplayState();

        const char* displayString() const;
        void update();

    private:
        DisplayState(const int displayId);

        int _displayId;
        String _displayString;
    };
}

#endif