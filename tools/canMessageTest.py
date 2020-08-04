import socket
from struct import *
import time

targetIP = '192.168.4.1'
#targetIP = '172.20.21.204'
targetPort = 9955


def sendFrame(id, data):
   headerBytes = pack('<ib', id, len(data))
   sock.sendto(headerBytes + data, (targetIP, targetPort)) 


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)


sendFrame(0x528, pack('>I', int(time.time())))
sendFrame(0x00C, pack('>Q', 0xFFFFFFFFFFFFFFFF))
time.sleep(1)

#sendFrame(0x00C, pack('>Q', 0))
#time.sleep(2)
##sendFrame(0x257, pack('>Q', 0xc50e2c0a28000000))

##exit(0)




voltageCounter = 35000
voltageInc = 10

ampsCounter = -10
ampsInc = -10
while (True):
    sendFrame(0x132, pack('<H', voltageCounter) + pack('<h', ampsCounter) + b'\x04\x27\xFF\x0F')
    time.sleep(0.008)
    if (voltageCounter > 36000):
        voltageInc *= -1
    elif (voltageCounter < 34000):
        voltageInc *= -1
        
    if (ampsCounter < -10000):
        ampsInc *= -1
    elif (ampsCounter > 4000):
        ampsInc *= -1
        
    ampsCounter += ampsInc
    voltageCounter += voltageInc
