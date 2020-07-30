import socket
from struct import *
import time

#targetIP = '192.168.4.1'
targetIP = '172.20.21.204'
targetPort = 9955


def sendFrame(id, data):
   headerBytes = pack('<ib', id, len(data))
   sock.sendto(headerBytes + data, (targetIP, targetPort)) 


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)


sendFrame(0x528, pack('>I', int(time.time())))
time.sleep(1)

lastFrameTime = 0
with open("/Users/chris/Downloads/CAN.raw (1).log") as f:
    for line in f:
        line = line.rstrip('\n')
        splitLine = line.split(" ")
        frameTime = splitLine[0][1:-1]
        
        if (lastFrameTime != 0):
            framediff = float(frameTime) - float(lastFrameTime)
            print(float(framediff))
            time.sleep(abs(float(framediff)))

        lastFrameTime = float(frameTime)
            
        splitFrameData = splitLine[2].split("#")
        paddedFrameId = splitFrameData[0].zfill(4)
        frameId = int.from_bytes(bytes.fromhex(paddedFrameId), "big")
        
        frameData = bytes.fromhex(splitFrameData[1])
        
        if (frameId == 0x528):
            #we skipp the time frame (so we get current times in logs and such)
            pass
        else:
            sendFrame(frameId, frameData)