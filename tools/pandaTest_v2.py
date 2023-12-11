import socket
import struct
import time
import threading
import struct
import datetime
from random import randrange
from pprint import pprint

#import cantools

#import hexdump

#targetIP = '192.168.4.1'
targetIP = '172.20.20.153'
targetPort = 1338

framesToFilter = [
    #[0, 0x132],
    #[0, 0x353],
    #[0, 0x332],
]

def extractValue(data_, valuedef):
    data = data_
    if valuedef['byteorder'] == 'little':
        pass
    elif valuedef['byteorder'] == 'big':
        tmpdata = struct.pack("<Q", data_)
        data = struct.unpack(">Q", tmpdata)[0]

    calculatedValue = ((1 << valuedef['bitlength']) - 1) & (data >> valuedef['bitstart'])
    if (valuedef['signed']):
        if (calculatedValue > pow(2, valuedef['bitlength']-1)):
            calculatedValue = 0 - (pow(2, valuedef['bitlength']) - calculatedValue)
        calculatedValue = float(calculatedValue) * valuedef['factor'] + valuedef['offset']
    else:
        calculatedValue = float(calculatedValue) * valuedef['factor'] + valuedef['offset']
    return calculatedValue

doshutdown = False
def heartbeatFunction(name):
    while True:
        #sock.sendto(b"hello", (targetIP, targetPort)) 
        if 1:
            time.sleep(4)
            print("sending ehllo")
            sock.sendto(b"ehllo", (targetIP, targetPort)) 

            #print("sending sendAll")
            #filterData = struct.pack("<B", 0x0C)
            #sock.sendto(filterData, (targetIP, targetPort))

            
            #print("sending heartbeat")
            #sock.sendto(b"ehllo", (targetIP, targetPort)) 
            #time.sleep(2)
            #print("sending bye")
            #sock.sendto(b"bye", (targetIP, targetPort)) 
            #time.sleep(2)
        else:
            print("sending hello")
            sock.sendto(b"hello", (targetIP, targetPort)) 
            time.sleep(2)

        if doshutdown:
            break


def filteringTest(name):
    filteringTestCounter = 0    
    while True:
        if (filteringTestCounter == 0):
            print("Clearing Filters")

            #clear any filters
            filterData = struct.pack("<B", 0x18)
            sock.sendto(filterData, (targetIP, targetPort))

            filteringTestCounter = 1
        elif (filteringTestCounter == 1):
            print("Adding Filters")

            #add some new filters
            filterData = struct.pack("!BBHBH", 0x0F, 0x01, 0x69, 0x00, 0x70 )
            sock.sendto(filterData, (targetIP, targetPort))
            filteringTestCounter = 2
        elif (filteringTestCounter == 2):
            print("Removing Filters")

            #add some new filters
            filterData = struct.pack("!BBH", 0x0E, 0x01, 0x69)
            sock.sendto(filterData, (targetIP, targetPort))
            filteringTestCounter = 0

        time.sleep(10)

def parsePandaPacket(data):
    packetStart = 0
    headerbytes = data[packetStart:packetStart + 8]

    #move past the header bytes
    packetStart = packetStart + 8

    unpackedHeader = struct.unpack('<II', headerbytes)
    frameID = unpackedHeader[0] >> 21

    #print(unpackedHeader[1])
    frameLength = unpackedHeader[1] & 0x0F
    frameBusId = unpackedHeader[1] >> 4 
    frameData = data[packetStart:packetStart+8]

    return (frameBusId, frameID, frameLength, frameData)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
random_port_offset = 0 #randrange(200)
used_port = targetPort+random_port_offset
print("Using random port " + str(used_port))
sock.bind(("0.0.0.0", used_port))

#Do initial connect
print("sending ehllo")
#sock.sendto(str.encode("ehllo {}".format(used_port)), (targetIP, targetPort))
sock.sendto(str.encode("ehllo"), (targetIP, targetPort)) 

#Wait for response
data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
parsedData = parsePandaPacket(data)
if parsedData[0] == 15 and parsedData[1] == 6:
    # We got a connection
    pass
elif parsedData[0] == 15 and parsedData[1] == 7:
    # Connection refused
    print("Connection refused")
    exit(1)
else:
    # Some other error....
    print("Failed to get a valid response")
    exit(1)

x = threading.Thread(target=heartbeatFunction, args=(1,))
x.start()

for filterEntry in framesToFilter:
    filterData = struct.pack("!BBH", 0x0F, filterEntry[0], filterEntry[1] )
    sock.sendto(filterData, (targetIP, targetPort))





doPrint = True
bytesReceived = 0
messageCount = 0

seenTracker = {0: {}, 1: {}, 8: {}, 15: {}}

#db = cantools.database.load_file('CANServer.dbc')

while True:
    try:
        data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
    
        dataLength = len(data)
        bytesReceived = bytesReceived + dataLength

        packetStart = 0
        
        while packetStart < dataLength:
            messageCount = messageCount + 1
            #Parse thes panda packets
            headerbytes = data[packetStart:packetStart + 8]

            #move past the header bytes
            packetStart = packetStart + 8

            unpackedHeader = struct.unpack('<II', headerbytes)
            frameID = unpackedHeader[0] >> 21

            #print(unpackedHeader[1])
            frameLength = unpackedHeader[1] & 0x0F
            frameBusId = unpackedHeader[1] >> 4 
            frameData = data[packetStart:packetStart+8]

    #        if (frameBusId == 15):
    #            doPrint = True
    #        else: 
    #            doPrint = False
            #lastSeen = 0
            #if (frameID in seenTracker[frameBusId]):
            #    lastSeen = seenTracker[frameBusId][frameID]
            #seenTracker[frameBusId][frameID] = round(time.time() * 1000)

            #print(frameBusId, frameID, seenTracker[frameBusId][frameID] - lastSeen)
            doPrint = True

            if (doPrint):
                print (int(time.time() * 1000), end='')
                print(" ", end='')
                print("can%d " % (frameBusId), end='')
                print("{0:03X}#".format(frameID), end='')
                #print("[%d]\t" % frameLength, end='')
                
                #panda packets are always 16 bytes.  8 header and 8 padded data            
                for payloadByte in frameData:
                    print("{0:02X} ".format(payloadByte), end='')

                print("")

            #if (frameBusId == 15):
            #    if (frameID == 0x504 or frameID == 0x502):
            #        pprint(db.decode_message(frameID, frameData))
            #        print("")
    #               unpackedData = struct.unpack("<Q", frameData)[0]

    #                if (frameid == )
    #                print("CAN 0 enabled:", extractValue(unpackedData, {'byteorder': 'little', 'bitlength': 4, 'bitstart': 0, 'signed': False, 'factor': 1, 'offset': 0}) == 1)
    #                print("CAN 1 enabled:", extractValue(unpackedData, {'byteorder': 'little', 'bitlength': 4, 'bitstart': 4, 'signed': False, 'factor': 1, 'offset': 0}) == 1)
    #                print("CAN 0 speed:", extractValue(unpackedData, {'byteorder': 'little', 'bitlength': 8, 'bitstart': 8, 'signed': False, 'factor': 5, 'offset': 0}))
    #                print("CAN 1 speed:", extractValue(unpackedData, {'byteorder': 'little', 'bitlength': 8, 'bitstart': 16, 'signed': False, 'factor': 5, 'offset': 0}))
    #                print("CAN 0 rate:", extractValue(unpackedData, {'byteorder': 'little', 'bitlength': 16, 'bitstart': 24, 'signed': False, 'factor': 1, 'offset': 0}))
    #                print("CAN 1 rate:", extractValue(unpackedData, {'byteorder': 'little', 'bitlength': 16, 'bitstart': 40, 'signed': False, 'factor': 1, 'offset': 0}))

            #move to the next packet (skipping any padding bytes)
            packetStart = packetStart + 8

        #print("Message Count: %d, Total Bytes: %d" % (messageCount, bytesReceived))
    except:
        sock.sendto(b"bye", (targetIP, targetPort)) 
        doshutdown = True
        break
