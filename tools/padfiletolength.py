import sys
import struct

if len(sys.argv) != 4:
    print("padfiletolength.py <filename> <lengthinbytes>")
    exit(1)

with open(sys.argv[1], mode='rb') as file:
    fileContent = file.read()
    contentLength = len(fileContent)

    sys.stdout.buffer.write(fileContent)

    for x in range(contentLength, int(sys.argv[3])):
        sys.stdout.buffer.write(struct.pack("B", int(sys.argv[2])))
    