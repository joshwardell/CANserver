import sys
import struct
import os.path
import argparse

# Construct an argument parser
all_args = argparse.ArgumentParser()

# Add arguments to the parser
all_args.add_argument("-i", "--input", required=True, action='append', help="Input File(s)")
all_args.add_argument("-o", "--output", required=True, help="Output File")
all_args.add_argument("-f", "--filter", required=False, action='append', help="Filter specific bus and frame ids")
all_args.add_argument("-s", "--split", required=False, action='store_true', help="Split output on log marks")
args = vars(all_args.parse_args())

original_stdout = sys.stdout

lastSyncTime = 0

outputfile = None
outputFilename = args['output']

filters = {}
dofilter = False

if "filter" in args:
    #Sort out the filters we have.    
    filterstring = args["filter"]
    if filterstring != None:
        dofilter = True
        for filterentry in filterstring:
            splitfilterentry = filterentry.split(",")
            busid = int(splitfilterentry[0])
            if busid not in filters:
                filters[busid] = []
            filters[busid].append(int(splitfilterentry[1], 16))

        print("Applying filters", filters)
import CANServer

logreader = CANServer.LogReader()
for inputfile in args["input"]:
    logreader.addFile(inputfile)


if (logreader.begin()):
    outputfile = open(outputFilename, mode='w')
    #sys.stdout = outputfile

    for frame in logreader.nextFrame():
        if frame.mark != None:
            if 'split' in args and args['split']:
                #lets switch to a new file named based on the mark
                splitOutputFilename = os.path.splitext(outputFilename)
                newOutputFilename = splitOutputFilename[0] + "-" + frame.mark + splitOutputFilename[1]
                
                outputfile.close()
                
                outputfile = open(newOutputFilename, mode='w')
        else:
            outputFrame = False
            if dofilter:
                if frame.busid in filters:
                    if frame.id in filters[frame.busid]:
                        outputFrame = True
            else:
                outputFrame = True

            if outputFrame:
                print("({0:017F})".format(frame.time/1000000), end='', file=outputfile)
                print(" can%d " % (frame.busid), end='', file=outputfile)
                print("{0:03X}#".format(frame.id), end='', file=outputfile)
                for payloadByte in frame.payload:
                    print("{0:02X}".format(payloadByte), end='', file=outputfile)
                print("", file=outputfile)

if (outputfile):
    outputfile.close()
sys.stdout = original_stdout