CANServer_address = "172.20.20.64"

import json 
from urllib import request, parse, error
import struct
import datetime
import time

sleepTime = 60

while True:
    #check to see if the server exists
    try:
        status = request.urlopen("http://" + CANServer_address + "/stats")

        print("CANServer alive.  Looking for files...")

        #if we got a response lets try and load the files list
        filesToDownload = []
        activeFileName = ""
        with request.urlopen("http://" + CANServer_address + "/logs/load") as url:
            data = json.loads(url.read().decode())
            activeFileName = data['activefile']

        offsetToFetchFrom = 0
        while (True):
            moreToFetch = False
            with request.urlopen("http://" + CANServer_address + "/logs/files?offset=" + str(offsetToFetchFrom)) as url:
                data = json.loads(url.read().decode())
                for filedata in data:   
                    if (filedata['n'] == activeFileName):
                        #ignore this file.  Its activly being logged to
                        pass
                    else:
                        if (filedata['n'] == '---cont---'):
                            moreToFetch = True
                            offsetToFetchFrom = filedata['s']
                        else:
                            filesToDownload.append(filedata['n'])

            if (moreToFetch == False):
                #We didn't get a new offset to fetch from, so we are all done
                break

        filesToDownload.sort()
        
        print("Found %d files..." % (len(filesToDownload)))
        exit(0)
        
        for fileToDownload in filesToDownload:
            downloadURL = "http://" + CANServer_address + "/logs/files/download?id=" + fileToDownload
            try:
                print("Downloading: " + fileToDownload + "...")
                filename, headers = request.urlretrieve(downloadURL, datetime.datetime.now().strftime("%Y-%m-%d-%H-%M") + " - " + fileToDownload)

                #download completed.  Delete the file
                data = parse.urlencode({ 'id': fileToDownload }).encode()
                req =  request.Request("http://" + CANServer_address + "/logs/files/delete", data=data)
                resp = request.urlopen(req)

            except error.URLError as e:
                print("Error downloading: " + fileToDownload)

        sleepTime = 60

    except error.URLError as e:
        #server isn't out there.
        sleepTime = 30
        pass

    time.sleep(sleepTime)



