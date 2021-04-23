import queue
import struct

class LogFrame:
    busid = 0
    id = 0
    time = 0
    length = 0
    payload = None

    def __init__(self, busid, id, time, length, payload):
        self.busid = busid
        self.id = id
        self.time = time
        self.length = length
        self.payload = payload

    def __str__(self):
        returnString =  "({0:017F})".format(self.time/1000000) + " can%d " % (self.busid) + "{0:03X}#".format(self.id) 
        for payloadByte in self.payload:
            returnString = returnString + "{0:02X}".format(payloadByte)

        return returnString

class LogReader:
    def __init__(self):
        self._filelist = queue.SimpleQueue()
        self._activefile = None

        self._lastSyncTime = 0

    def addFile(self, filename):
        self._filelist.put(filename)

    def _validateHeader(self):
        if self._activefile != None and self._activefile.closed == False:
            headerData = self._activefile.read(22)
            if (len(headerData) == 22):
                #Check to see if our header matches what we expect        
                if (headerData == b'CANSERVER_v2_CANSERVER'):
                    return True
        
        print("not a canserver log")
        return False

    def begin(self):
        return self._nextFile()

    def _nextFile(self):
        # Open the next file in the list and verify that it is a sane CANServer log file
        if self._activefile != None and self._activefile.closed == False:
            self._activefile.close()

        if self._filelist.empty():
            #nothing more to load
            return False

        try:
            filename = self._filelist.get_nowait()
            self._activefile = open(filename, mode='rb')

            return self._validateHeader()

        except:
            return False
        

    def nextFrame(self):
        while True:
            byteRead = self._activefile.read(1)
            if len(byteRead) == 1:
                if (byteRead == b'C'):
                    # Check for a new header (might be concationated files)
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
                        self._activefile.seek(-len(possibleHeader), 1)

                elif (byteRead == b'\xcd'):
                    #this is a mark message.
                    marksize = self._activefile.read(1)
                    marksize = int.from_bytes(marksize, 'big')
                    markdata = self._activefile.read(marksize)

                    markString = markdata.decode("ascii")

                    #TODO return this in a meaningfull way
                    
                elif (byteRead == b'\xce'):
                    #this is running time sync message.
                    timesyncdata = self._activefile.read(8)

                    if len(timesyncdata) == 8:
                        self._lastSyncTime = struct.unpack('<Q', timesyncdata)[0]
                    else:
                        print("Time Sync frame read didn't return the proper number of bytes", file=sys.stderr)

                    pass
                elif (byteRead == b'\xcf'):
                    #this is a can frame
                    #we found our start byte.  Read another 5 bytes now
                    framedata = self._activefile.read(5)
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

                        framepayload = self._activefile.read(framelength)

                        frametime = self._lastSyncTime + frametimeoffset

                        yield LogFrame(busid, frameid, frametime, framelength, framepayload)
                    else:
                        print("Skipping byte")
            else:
                # we weren't able to read any more data.  Lets try and move on to the next file
                if (not self._nextFile()):
                    #if we can't open any more files then bail out
                    break
