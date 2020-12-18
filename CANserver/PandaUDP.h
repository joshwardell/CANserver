
#ifndef __PANDAUDP_H__
#define __PANDAUDP_H__

#include <Arduino.h>
#include <AsyncUDP.h>
#include "esp32_can/esp32_can.h"

class AsyncUDP;

class PandaUDP {

protected:
    AsyncUDP udp;

    uint32_t  timeout = 0;

    uint16_t localPort = 0;

    IPAddress _remoteIP;
    uint16_t  _remotePort = 0;

public:

    void begin(uint16_t localPort = 1338);
    void handleMessage(CAN_FRAME message, const uint8_t busId);

    const IPAddress* remoteIP() const { return &_remoteIP; }
    const uint16_t remotePort() const { return _remotePort; }
	
	~PandaUDP();
};


typedef struct {
    uint32_t f1;
    uint32_t f2;
    uint8_t  data[8];
} PandaPacket;

#endif
