import queue
import struct
import os
import io

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

    @property
    def length(self):
        return self._length
    
class LogReader:
    def __init__(self):
        self._filelist = queue.SimpleQueue()
        self._activefile = None
        self._activefileobj = None

        self._lastSyncTime = 0

        self.totalbytes = 0
        self.readbytes = 0

    def close(self):
        if self._activefileobj != None and self._activefileobj.closed == False:
            self._activefileobj.close()
            self._activefile = None

        self._filelist = queue.SimpleQueue()

    def addFile(self, filename):
        self.totalbytes += os.path.getsize(filename)
        self._filelist.put(filename)

    def _validateHeader(self):
        if self._activefile != None and self._activefile.closed == False:
            headerData = self.__read(22)
            if (len(headerData) == 22):
                #Check to see if our header matches what we expect
                if (headerData == b'CANSERVER_v2_CANSERVER'):
                    return True

        print("not a canserver log")
        return False

    def begin(self):
        return self._nextFile()

    def __read(self, bytecount):
        self.readbytes += bytecount
        return self._activefile.read(bytecount)

    def _nextFile(self):
        # Open the next file in the list and verify that it is a sane CANServer log file
        if self._activefileobj != None and self._activefileobj.closed == False:
            self._activefileobj.close()
            self._activefile = None

        if self._filelist.empty():
            #nothing more to load
            return False

        try:
            filename = self._filelist.get_nowait()
            self._activefileobj = open(filename, mode='rb')
            self._activefile = io.BufferedReader(io.FileIO(self._activefileobj.fileno()), buffer_size = 8192*1024)

            return self._validateHeader()

        except:
            return False

    def nextFrame(self):
        while True:
            byteRead = self.__read(1)
            #print(byteRead)
            if byteRead == b'C':
                # Check for a new header (might be concationated files)
                goodheader = False

                #read 21 more bytes
                possibleHeader = self._activefile.peek(21)
                if possibleHeader[:21] == b'ANSERVER_v2_CANSERVER':
                    #we found a header (this might have been because of just joining multiple files togeather)
                    #we can safely skip these bytes
                    goodheader = True
                    pass


                if goodheader:
                    #header was valid.  Just skip on ahead

                    #since we only peek'ed at this data we need to actually consume it now
                    self.__read(21)
                    pass
                else:
                    #we didn't see the header we expected.
                    pass

            elif byteRead == b'\xcd':
                #this is a mark message.
                marksize = self.__read(1)
                marksize = int.from_bytes(marksize, 'big')
                markdata = self.__read(marksize)

                markString = markdata.decode("ascii")

                #TODO return this in a meaningfull way

            elif byteRead == b'\xce':
                #this is running time sync message.
                timesyncdata = self.__read(8)

                if timesyncdata != b'':
                    self._lastSyncTime = struct.unpack('<Q', timesyncdata)[0]
                else:
                    print("Time Sync frame read didn't return the proper number of bytes", file=sys.stderr)

                pass
            elif byteRead == b'\xcf':
                #this is a can frame
                #we found our start byte.  Read another 5 bytes now
                framedata = self.__read(5)
                if framedata != b'':
                    unpackedFrame = struct.unpack('<HHB', framedata)
                    
                    #convert the frametimeoffset  from ms to us
                    frametimeoffset = unpackedFrame[0] * 1000

                    framelength_and_busid = unpackedFrame[2]
                    
                    busid = (framelength_and_busid & 0xf0) >> 4

                    framelength = framelength_and_busid & 0x0f
                    if (framelength < 0):
                        framelength = 0
                    elif (framelength > 8):
                        framelength = 8

                    framepayload = self.__read(framelength)

                    frametime = self._lastSyncTime + frametimeoffset

                    yield LogFrame(busid, unpackedFrame[1], frametime, framelength, framepayload)
                else:
                    print("Skipping byte")
            elif byteRead != b'':
                #lets just skip this byte cause we fell through this far so its not something we expected
                print(byteRead)
                pass
            else:
                # we weren't able to read any more data.  Lets try and move on to the next file
                if (not self._nextFile()):
                    #if we can't open any more files then bail out
                    break
