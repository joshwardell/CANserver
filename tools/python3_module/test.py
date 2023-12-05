import CANServer

import struct

logreader = CANServer.LogReader()
logreader.addFile("/Users/chris/Downloads/R_0001-001.log")

if (logreader.begin()):
    for frame in logreader.nextFrame():
        print(frame)