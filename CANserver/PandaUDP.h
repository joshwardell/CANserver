
#ifndef __PANDAUDP_H__
#define __PANDAUDP_H__

#include <arduino.h>
#include <AsyncUDP.h>
#include "esp32_can/esp32_can.h"

class AsyncUDP;

#define PANDA_SRC_BUS_ID 2

class PandaUDP {

protected:
    AsyncUDP udp;

    uint32_t  timeout = 0;

    uint16_t localPort = 0;

    IPAddress remoteIP;
    uint16_t  remotePort = 0;

public:

    void begin(uint16_t localPort = 1338);
    void handleMessage(CAN_FRAME message);
	
	~PandaUDP();
};


typedef struct {
    uint32_t f1;
    uint32_t f2;
    uint8_t  data[8];
} PandaPacket;

#endif