import CANServer

logreader = CANServer.LogReader()

logreader.addFile("/Users/chris/Desktop/2021.4.12/20210408_142231-R_0001-001.log")
logreader.addFile("/Users/chris/Desktop/2021.4.12/20210408_142746-R_0001-002.log")
logreader.addFile("/Users/chris/Desktop/2021.4.12/20210408_143257-R_0001-003.log")
logreader.addFile("/Users/chris/Desktop/2021.4.12/20210408_143813-R_0001-004.log")
logreader.addFile("/Users/chris/Desktop/2021.4.12/20210408_144331-R_0001-005.log")

if (logreader.begin()):
    for frame in logreader.nextFrame():
        print(frame)