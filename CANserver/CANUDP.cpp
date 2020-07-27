#include "CANUDP.h"

typedef struct 
{
	uint32_t id;
	uint8_t length;
	uint64_t data;
} debug_can_frame_t;

CANServer::CANUDP::CANUDP()
{
	_newFrame = false;
	memset(&_frame, 0, sizeof(CAN_FRAME));
}

CANServer::CANUDP::~CANUDP() {
	udp.close();
}

void CANServer::CANUDP::begin(uint16_t localPort) 
{
	if (udp.listen(localPort)) {
		udp.onPacket([this](AsyncUDPPacket packet) {
      		debug_can_frame_t inboundData;
			packet.read((uint8_t*) &(inboundData.id), 4);
			packet.read((uint8_t*) &(inboundData.length), 1);

			//Make sure to clamp the inbound data length to a max of 8
			inboundData.length = min((int)inboundData.length, 8);

			packet.read((uint8_t*) &(inboundData.data), inboundData.length);

			_frame.id = inboundData.id;
			_frame.length = inboundData.length;
			memcpy(_frame.data.bytes, &(inboundData.data), inboundData.length);

			_newFrame = true;
		});
	}
}



const bool CANServer::CANUDP::read(CAN_FRAME &message)
{
	if (_newFrame)
	{
		memcpy(&message, &_frame, sizeof(CAN_FRAME));
		
		_newFrame = false;
		memset(&_frame, 0, sizeof(CAN_FRAME));

		return true;
	}
	return false;
}
