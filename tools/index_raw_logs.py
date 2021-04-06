#!/usr/bin/env python3
# v1.1 April 6 2021 balt@inside.net
import sys
import struct
from datetime import datetime
import glob
import os
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-l', '-dir', action='store', dest='logdir', required=True,
                    help='directory containing log files')

parser.add_argument('-c', '--cachefile', action='store', dest='cachefile', default='logcache.json', required=False,
                    help='JSON file containing the cached entries')

parser.add_argument('-p', '--prune', action='store_true', dest='prune', required=False,
                    help='If set, removes index entries for log files it doesn\'t find')

myargs = parser.parse_args()

logcache = {}

try:
  with open(myargs.cachefile, 'r') as f:
    lcstring = f.read()
    if len(lcstring) > 0:
      logcache = json.loads(lcstring)
except FileNotFoundError:
  pass

if not os.path.isdir(myargs.logdir):
  print("%s is not a directory!" % myargs.logdir)
  sys.exit(1)

def logsummary(filename):
  startTime = sys.maxsize
  endTime = 0
  lastSyncTime = 0
  framecount = 0

  with open(filename, mode='rb') as file:

    #File header should be 22 bytes
    headerData = file.read(22)
    if (len(headerData) == 22):
        #Check to see if our header matches what we expec
        if (headerData == b'CANSERVER_v2_CANSERVER'):
            #all is well
            pass
        else:
            print("Not a valid CANServer v2 file.  Unable to convert", file=sys.stderr)
            raise

    #print("Analysing file...")

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
                    if (frametime < startTime and lastSyncTime > 1e7):
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

  dt_start = datetime.fromtimestamp(startTime/1000/1000)
  dt_end = datetime.fromtimestamp(endTime/1000/1000)

  return (dt_start, dt_end, framecount)

broken = 0
indexed = 0
cached = len(logcache)
fileno = 0
totalframes = 0
totaltime = datetime.now() - datetime.now()

brokendir = myargs.logdir + "/broken"

files = glob.glob(myargs.logdir + "/*.log")
for file in sorted(files):
  fileno += 1
  try:
    basename = os.path.basename(file)
    # cached?
    if basename in logcache:
      print("%s (cached):" % basename)
      print("%s\n%s %d frames\n" % (datetime.fromtimestamp(logcache[basename]['dt_start']), datetime.fromtimestamp(logcache[basename]['dt_end']), logcache[basename]['framecount']))
      totalframes += logcache[basename]['framecount']
      totaltime += datetime.fromtimestamp(logcache[basename]['dt_end']) - datetime.fromtimestamp(logcache[basename]['dt_start'])
    else:
      indexed += 1
      print("%s (%d/%d):" % (basename, fileno, len(files)))
      try:
        dt_start, dt_end, framecount = logsummary(file)
        print("%s\n%s %d frames\n" % (dt_start, dt_end, framecount))

        # adjust the filename from the 'downlaoded timestamp - R_XXXX-XXX.log' to 'start_timestamp-R_XXXX-XXX.log'
        logserial = basename[-14:]
        newbasename = "%s-%s" % (dt_start.strftime("%Y%m%d_%H%M%S"), logserial)
        logcache[newbasename] = {}
        logcache[newbasename]['dt_start'] = datetime.timestamp(dt_start)
        logcache[newbasename]['dt_end'] = datetime.timestamp(dt_end)
        logcache[newbasename]['framecount'] = framecount
        totalframes += framecount
        totaltime += dt_end - dt_start
        # rename the file
        os.rename("%s/%s" % (myargs.logdir, basename), "%s/%s" % (myargs.logdir, newbasename))
      except Exception as e:
        broken += 1
        print("Error %s while processing %s" % (str(e), basename))
        if not os.path.isdir(brokendir):
          os.mkdir(brokendir)
        # move the file to the broken dir
        os.rename(file, brokendir + "/" + basename)

  except KeyboardInterrupt:
    break

if myargs.prune:
  dellist = []
  # check each file, if it doesn't exist remove the entry
  count = 0
  for logfile in logcache:
    if not os.path.isfile(myargs.logdir + "/" + logfile):
      dellist.append(os.path.basename(logfile))

  for file in dellist:
    del logcache[file]
    count += 1

  print("Pruned %d index entries" % count)

# save the log index json
with open(myargs.cachefile, 'w') as f:
  f.write(json.dumps(logcache, sort_keys=True))

print("Cached: %d Indexed: %d. Broken: %d Total frames: %s Total time: %s" % (len(files), indexed, broken, f'{totalframes:,}', totaltime))




















