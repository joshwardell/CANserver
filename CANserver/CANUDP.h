
#ifndef __CANUDP_H__
#define __CANUDP_H__

#include <Arduino.h>
#include <AsyncUDP.h>
#include <esp32_can.h>

class AsyncUDP;

namespace CANServer 
{
    class CANUDP {
        protected:
            AsyncUDP udp;

            CAN_FRAME _frame;
            bool _newFrame;

        public:

            void begin(uint16_t localPort = 9955);
            const bool read(CAN_FRAME &message);
            
            CANUDP();
            ~CANUDP();
    };
}

#endif