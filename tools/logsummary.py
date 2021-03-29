import sys
import struct
from datetime import datetime

if len(sys.argv) != 2:
    print("logsummary.py <infile>")
    exit(1)

original_stdout = sys.stdout

lastSyncTime = 0

outputfile = None

framecount = 0

startTime = sys.maxsize
endTime = 0

with open(sys.argv[1], mode='rb') as file:
    
    #File header should be 22 bytes
    headerData = file.read(22)
    if (len(headerData) == 22):
        #Check to see if our header matches what we expec        
        if (headerData == b'CANSERVER_v2_CANSERVER'):
            #all is well
            pass
        else:
            print("Not a valid CANServer v2 file.  Unable to convert", file=sys.stderr)
            exit(1)

    print("Analysing file...")

    while True:
        #Look for the start byte
        byteRead = file.read(1)
        if len(byteRead) == 1:
            if (byteRead == b'C'):
                #check to see if we have a header.
                goodheader = False
                
                #read 21 more bytes
                possibleHeader = file.read(21)
                if (len(possibleHeader) == 21):
                    if (possibleHeader == b'ANSERVER_v2_CANSERVER'):
                        #we found a header (this might have been because of just joining multiple files togeather)
                        #we can safely skip these bytes
                        goodheader = True
                        pass

                if (goodheader):
                    #header was valid.  Just skip on ahead
                    pass
                else:
                    #we didn't see the header we expected.  Seek backwards the number of bytes we read
                    file.seek(-len(possibleHeader), 1)                    


            elif (byteRead == b'\xce'):
                #this is running time sync message.
                timesyncdata = file.read(8)

                if len(timesyncdata) == 8:
                    lastSyncTime = struct.unpack('<Q', timesyncdata)[0]                    
                else:
                    print("Time Sync frame read didn't return the proper number of bytes")

            elif (byteRead == b'\xcf'):
                #we found our start byte.  Read another 5 bytes now
                framedata = file.read(5)
                if len(framedata) == 5:
                    unpackedFrame = struct.unpack('<2cHB', framedata)

                    frametimeoffset = int.from_bytes(unpackedFrame[0] + unpackedFrame[1], 'little')
                    #convert the frametimeoffset  from ms to us
                    frametimeoffset = frametimeoffset * 1000

                    frameid = unpackedFrame[2]

                    framelength = unpackedFrame[3] & 0x0f
                    busid = (unpackedFrame[3] & 0xf0) >> 4
                    if (framelength < 0):
                        framelength = 0
                    elif (framelength > 8):
                        framelength = 8

                    framepayload = file.read(framelength)

                    frametime = lastSyncTime + frametimeoffset

                    if (frametime < startTime):
                        startTime = frametime
                    
                    if (frametime > endTime):
                        endTime = frametime

                    #print("({0:017F})".format(frametime/1000000), end='')
                    #print(" can%d " % (busid), end='')
                    #print("{0:03X}#".format(frameid), end='')
                    #for payloadByte in framepayload:
                    #    print("{0:02X}".format(payloadByte), end='')
                    #print("")
                    framecount = framecount + 1

                else:
                    break
        else:
            break

dt_object = datetime.fromtimestamp(startTime/1000/1000)
print("startTime: ", dt_object)
dt_object = datetime.fromtimestamp(endTime/1000/1000)
print("endTime: ", dt_object)
print("framecount: ", framecount)