#!/usr/bin/env python3

import sys
import struct
from numpy import mean
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
import argparse
import pytz
import os

parser = argparse.ArgumentParser()
parser.add_argument('-l', '--log', action='store', dest='rawlog', required=True,
                    help='full path and name of raw log file to check')

myargs = parser.parse_args()

bus_chassis = 0
bus_vehicle = 1
bus_powertrain = 1

lastSyncTime = 0
lastSecond = -1
countSecond = 0
frame318counter = []

bus0frameids = {}
bus1frameids = {}
bus15frameids = {}

startTime = sys.maxsize
endTime = 0
timedeltas = []
xaxis = []

car_model3 = False
car_models = True

lastFrameOffset = 0
countSinceSync = 0
framecount = 0 
synccount = 0

def extractValue(data, valuedef):
    calculatedValue = ((1 << valuedef['bitlength']) - 1) & (data >> valuedef['bitstart'])
    if (valuedef['signed']):
        if (calculatedValue > pow(2, valuedef['bitlength']-1)):
            calculatedValue = 0 - (pow(2, valuedef['bitlength']) - calculatedValue)
        calculatedValue = float(calculatedValue) * valuedef['factor'] + valuedef['offset'];
    else:
        calculatedValue = float(calculatedValue) * valuedef['factor'] + valuedef['offset']
    return calculatedValue

with open(myargs.rawlog, mode='rb') as file:

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

    while True:
        #Look for the start byte
        byteRead = file.read(1)
        if len(byteRead) == 1:            
            if (byteRead == b'C'):
                print("Sub Header")
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

                    synccount = synccount + 1
                    print("Sync# %d - %d" % (synccount, lastSyncTime))      

                    
                    countSinceSync = 0      
                    lastFrameOffset = 0       
                else:
                    print("Time Sync frame read didn't return the proper number of bytes")

            elif (byteRead == b'\xcf'):
                #we found our start byte.  Read another 5 bytes now
                framedata = file.read(5)
                if len(framedata) == 5:
                    framecount = framecount + 1
                    unpackedFrame = struct.unpack('<2cHB', framedata)

                    framesyncoffset = int.from_bytes(unpackedFrame[0] + unpackedFrame[1], 'little')
                    #convert the frametimeoffset  from ms to us
                    framesyncoffset = framesyncoffset * 1000

                    countSinceSync = countSinceSync + 1

                    if (lastFrameOffset != 0 and lastFrameOffset > framesyncoffset):
                        pass
                        #print("Frame went back in time - countSinceSync: %d, lastFrameOffset: %08d > framesyncoffset: %08d" % (countSinceSync, lastFrameOffset, framesyncoffset))
                    
                    lastFrameOffset = framesyncoffset

                    frameid = unpackedFrame[2]

                    framelength = unpackedFrame[3] & 0x0f
                    busid = (unpackedFrame[3] & 0xf0) >> 4
                    if (framelength < 0):
                        framelength = 0
                    elif (framelength > 8):
                        framelength = 8

                    framepayload = file.read(framelength)

                    frametime = lastSyncTime + framesyncoffset
                    if (frametime < startTime):
                        startTime = frametime

                    if (frametime > endTime):
                        endTime = frametime

                    if ((car_model3 and busid == bus_chassis) or (car_models and busid == bus_chassis)):
                      if frameid == 0x318: # 792 GTW_carState
                        
                        frameuint64 = struct.unpack('<Q', framepayload)[0]
                        year = 0
                        month = 0
                        day = 0
                        hour = 0
                        minute = 0
                        second = 0

                        if (car_models):
                            #model S
                            year = extractValue(frameuint64, {'bitstart': 0, 'bitlength': 7, 'factor': 1, 'offset': 2000, 'signed': False})
                            month = extractValue(frameuint64, {'bitstart': 8, 'bitlength': 4, 'factor': 1, 'offset': 0, 'signed': False})
                            day = extractValue(frameuint64, {'bitstart': 32, 'bitlength':5, 'factor': 1, 'offset': 0, 'signed': False})
                            hour = extractValue(frameuint64, {'bitstart': 24, 'bitlength': 5, 'factor': 1, 'offset': 0, 'signed': False})
                            minute = extractValue(frameuint64, {'bitstart': 40, 'bitlength': 6, 'factor': 1, 'offset': 0, 'signed': False})
                            second = extractValue(frameuint64, {'bitstart': 16, 'bitlength': 6, 'factor': 1, 'offset': 0, 'signed': False})
                        else:
                            #model 3
                            year = extractValue(frameuint64, {'bitstart': 0, 'bitlength': 8, 'factor': 1, 'offset': 2000, 'signed': False})
                            month = extractValue(frameuint64, {'bitstart': 8, 'bitlength': 8, 'factor': 1, 'offset': 0, 'signed': False})
                            day = extractValue(frameuint64, {'bitstart': 32, 'bitlength': 8, 'factor': 1, 'offset': 0, 'signed': False})
                            hour = extractValue(frameuint64, {'bitstart': 24, 'bitlength': 8, 'factor': 1, 'offset': 0, 'signed': False})
                            minute = extractValue(frameuint64, {'bitstart': 40, 'bitlength': 8, 'factor': 1, 'offset': 0, 'signed': False})
                            second = extractValue(frameuint64, {'bitstart': 16, 'bitlength': 8, 'factor': 1, 'offset': 0, 'signed': False})


                        ftime = datetime.utcfromtimestamp(frametime/1000/1000).replace(tzinfo=pytz.timezone('UTC'))
                        x318time = datetime(int(year), int(month), int(day), int(hour), int(minute), int(second)).replace(tzinfo=pytz.timezone('UTC'))

                        frameTimeDifference = (ftime - x318time).total_seconds()
                        if ((frameTimeDifference > 1 or frameTimeDifference < -0.05)): #or (synccount >= 37 and synccount < 39)
                            print("#: %d - offset(us): %08d - ftime(us): %d - ftime: %s - x318time: %s - diff: %f" % (framecount, framesyncoffset, frametime, ftime.strftime("%Y-%m-%d %H:%M:%S"), x318time.strftime("%Y-%m-%d %H:%M:%S"), frameTimeDifference))                        



                        timedeltas.append(frameTimeDifference)
                        xaxis.append(ftime)
                        
                        countSecond += 1
                        if second != lastSecond:
                          lastSecond = second
                          frame318counter.append(countSecond)
                          countSecond = 0


                else:
                    break
            else:
                print("Eating byte")
        else:
            break

print(frame318counter)
print("min: %d max: %d avg: %0.1f" % (min(frame318counter), max(frame318counter), mean(frame318counter)))
print(min(timedeltas), max(timedeltas))
print(min(xaxis), max(xaxis))

fig, ax1 = plt.subplots(nrows=1, sharex=True, figsize=(20,10))
scatter = ax1.plot(xaxis, timedeltas, marker='.')
ax1.grid(linestyle="--", color='black', alpha=0.1)
ax1.set_xlim(min(xaxis), max(xaxis))
ax1.set_xlabel("frame time")
ax1.set_ylabel("frametime - x318time [s]")

plt.tight_layout(pad=3)

splitLogFileName = os.path.splitext(os.path.basename(myargs.rawlog))
fig.savefig(splitLogFileName[0] + '_timing.png')
plt.close()


