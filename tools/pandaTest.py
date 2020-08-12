import socket
from struct import *
import time
import threading

#targetIP = '192.168.4.1'
targetIP = '172.20.21.204'
targetPort = 1338

def helloFunction(name):
    while True:
        sock.sendto(b"hello", (targetIP, targetPort)) 
        time.sleep(4)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.bind(("0.0.0.0", targetPort))

x = threading.Thread(target=helloFunction, args=(1,))
x.start()

while True:
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
    print("received message: %s" % data)
