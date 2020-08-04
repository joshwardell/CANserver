#!/usr/local/bin/python3

import time
import re
import sys
import math

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

if (len(sys.argv) != 2):
    print ("Requires display ID as a command line argument")
    exit(1)

print ('Argument List:', str(sys.argv))

import http.client
conn = http.client.HTTPConnection("192.168.4.1")

pattern = re.compile(".*?(?P<rate>\d+)r")

while(True):
    start = time.time()
    print()
    conn.request("GET", "/disp" + sys.argv[1])
    r1 = conn.getresponse()
    displayBytes = r1.read()
    end = time.time()
    print("Request time: ", math.floor((end - start) * 1000), "ms")

    displayString = displayBytes.decode("utf-8")
    print(displayString)

    #parse the string to find the value and the refresh time
    matchObj = pattern.fullmatch(displayString)
    refreshRate = matchObj.group('rate')

    if (math.floor((end-start) * 1000) > int(refreshRate)):
        print(f"{bcolors.FAIL}Using rate: : {refreshRate} ms {bcolors.ENDC}")
    else:
        print("Using rate: ", refreshRate, "ms")    
    time.sleep(float(refreshRate)/1000.0)


conn.close()
