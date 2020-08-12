#include "PandaUDP.h"

// MIT License

// Copyright (c) 2020 Jake Bordens

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This class will emulate the UDP streaming protocol of the Comma.ai Panda OBD-2 interface.

// 1. a UDP clients hould send a UDP packet with the contents "hello".
//
// 2. the Panda responds a UDP packet containing between 1-512 frames of the following format:
//                      Panda UDP Packet format                     
//                 │               │                                
// ┌───┬───┬───┬───┼───┬───┬─┬─┬─┬─┼───┬───┬───┬───┬───┬───┬───┬───┐
// │   │   │   │   │   │   │   │   │   │   │   │   │   │   │   │   │
// └───┴───┴───┴───┴───┴───┴─┴─┴─┴─┴───┴───┴───┴───┴───┴───┴───┴───┘
// │              ││         │   │ │                                
// └─────┴────────┘└─────?───┴─▲─┘▲└─────────────────────▶          
// 11-bits or 29-bits          │  │        Payload data             
//      Address                │  │                                 
//                             │  │                                 
//                     Bus ID ─┘  │                                 
//              Payload Length────┘                                 
//
// 3. After 5 seconds, the Panda will stop trasmitting data.   If a client wishes to continue
//    streaming, the client should send a UDP "hello" request at a minimum once every 5 seconds.
//

void PandaUDP::begin(uint16_t localPort_) {

  localPort = localPort_;

	// Setup the UDP server
	if (udp.listen(localPort)) {
		udp.onPacket([this](AsyncUDPPacket packet) {
			// You could do a check here to see if the client sent "hello"
			// I'm lazy and will just assume any packet arriving here is 
			// a request to start/continue streaming

            // Set the remote port and IP
            _remoteIP = packet.remoteIP();
            _remotePort = packet.remotePort();

            Serial.print("Panda packet from: ");
            Serial.println(_remoteIP);
            
			// Set the timeout to 5 seconds in the future.
			timeout = millis() + 10000;
		});
	}
}

PandaPacket p;
volatile bool sendActive = false;
void PandaUDP::handleMessage(CAN_FRAME message, const uint8_t busId) {
	// If remote port == 0, then we do not have an active client connected.
  if (_remotePort > 0) 
  {
    if (millis() > timeout) 
    {
      Serial.println("Closing Panda stream due to timeout");
      _remotePort = 0;
    } 
    else 
    {
      if (!sendActive)
      {
        sendActive = true;
        //Load the packet        
        p.f1 = message.id << 21;
        p.f2 = (message.length & 0x0F) | (busId << 4);
        memcpy(p.data, message.data.byte, message.length);        

        //Send to the client
        udp.writeTo((uint8_t*)&p, sizeof(PandaPacket), _remoteIP, localPort);
        sendActive = false;
      }
    }
  }
}

PandaUDP::~PandaUDP() {
	// Clean up.  In practice the PandaUDP object is kept in memory indefintely.
	udp.close();
}
