import queue
import struct

class LogFrame:
    _busid = 0
    _id = 0
    _time = 0
    _length = 0
    _payload = None

    def __init__(self, busid, id, time, length, payload):
        self._busid = busid
        self._id = id
        self._time = time
        self._length = length
        self._payload = payload

    def __str__(self):
        returnString =  "({0:017F})".format(self.time/1000000) + " can%d " % (self.busid) + "{0:03X}#".format(self.id)
        for payloadByte in self.payload:
            returnString = returnString + "{0:02X}".format(payloadByte)

        return returnString

    @property
    def id(self):
        return self._id

    @property
    def busid(self):
        return self._busid

    @property
    def payload(self):
        return self._payload

    @property
    def time(self):
        return self._time

class LogReader:
    def __init__(self):
        self._filelist = queue.SimpleQueue()
        self._activefile = None

        self._lastSyncTime = 0

        self._log_version = 1

    def addFile(self, filename):
        self._filelist.put(filename)

    def _validateHeader(self):
        if self._activefile != None and self._activefile.closed == False:
            headerData = self._activefile.read(22)
            if (len(headerData) == 22):
                #Check to see if our header matches what we expect
                if (headerData == b'CANSERVER_v2_CANSERVER'):
                    self._log_version = 1
                    return True
                if (headerData == b'CANSERVER_v3_CANSERVER'):
                    self._log_version = 2
                    return True

        print("Not a canserver log")
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
                            #we found a header (this might have been because of just joining multiple files together)
                            #we can safely skip these bytes
                            self._log_version = 1
                            goodheader = True
                        if (possibleHeader == b'ANSERVER_v3_CANSERVER'):
                            #we found a header (this might have been because of just joining multiple files together)
                            #we can safely skip these bytes
                            self._log_version = 2       # Technically you can concat multiple versions of log files together and things should work
                            goodheader = True


                    if (goodheader):
                        #header was valid.  Just skip on ahead
                        pass
                    else:
                        #we didn't see the header we expected.  Seek backwards the number of bytes we read
                        self._activefile.seek(-len(possibleHeader), 1)

                elif (self._log_version == 1 and byteRead == b'\xcd') or (self._log_version == 2 and byteRead == b'\xd0'):
                    #this is a mark message.
                    marksize = self._activefile.read(1)
                    marksize = int.from_bytes(marksize, 'big')
                    markdata = self._activefile.read(marksize)

                    markString = markdata.decode("ascii")

                    #TODO return this in a meaningful way

                elif (self._log_version == 1 and byteRead == b'\xce') or (self._log_version == 2 and byteRead == b'\xa0'):
                    #this is running time sync message.
                    timesyncdata = self._activefile.read(8)

                    if len(timesyncdata) == 8:
                        self._lastSyncTime = struct.unpack('<Q', timesyncdata)[0]
                    else:
                        print("Time Sync frame read didn't return the proper number of bytes", file=sys.stderr)

                    pass
                elif (self._log_version == 1 and byteRead == b'\xcf'):
                    #this is a can frame (from a version 1 log)
                    #we found our start byte.  Read another 5 bytes now
                    framedata = self._activefile.read(5)
                    if len(framedata) == 5:
                        unpackedFrame = struct.unpack('<2cHB', framedata)

                        frametimeoffset = int.from_bytes(unpackedFrame[0] + unpackedFrame[1], 'little')
                        if (unpackedFrame[2] == 0x557):
                            print(unpackedFrame[2], frametimeoffset)
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

                elif (self._log_version == 2 and byteRead[0] & 0b11110000 == 0xB0):
                    #this is a can frame (from a version 2 log)
                    #we found our start byte.  Read another 5 bytes now
                    framedata = self._activefile.read(5)
                    if len(framedata) == 5:

                        unpackedFrame = struct.unpack('<BBBBB', framedata)
                        frametimeoffset = (byteRead[0] & 0x0F) + (unpackedFrame[0] << 4) + (unpackedFrame[1] << 12) + (((unpackedFrame[2] & 0xF8) >> 3) << 20)

                        frametime = self._lastSyncTime + frametimeoffset

                        frameid = ((unpackedFrame[2] & 0x07) << 8) + (unpackedFrame[3] & 0xFF)

                        framelength = (unpackedFrame[4] & 0x0F)
                        busid = (unpackedFrame[4] & 0xF0) >> 4
                        if (framelength < 0):
                            framelength = 0
                        elif (framelength > 8):
                            framelength = 8

                        framepayload = self._activefile.read(framelength)

                        yield LogFrame(busid, frameid, frametime, framelength, framepayload)
                else:
                    print("Skipping byte", byteRead, byteRead[0] & 0b11110000)
            else:
                # we weren't able to read any more data.  Lets try and move on to the next file
                if (not self._nextFile()):
                    #if we can't open any more files then bail out
                    break
