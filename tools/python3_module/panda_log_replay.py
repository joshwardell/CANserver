from http import client
import sys
import struct
import os.path

import time
import threading
import socket
import struct
import logging
logging.basicConfig(level=logging.DEBUG)

if len(sys.argv) != 2:
    print("panda_log_replay.py <logfile> ")
    exit(1)

import CANServer

logreader = CANServer.LogReader()
logreader.addFile(sys.argv[1])

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_socket.setblocking(0)


shutdownFlag = False
connected_client_addr = None
clientVersion = 1
clientConnected = False
clientLastSeen = 0

v2SendAll = False

_filterList = [{}, {}]

def _addFilterForBusAndFrameId(busid, frameid):
    global _filterList

    if frameid not in _filterList[busid]:
        _filterList[busid][frameid] = True

def _removeFilterForBusAndFrameId(busid, frameid):
    global _filterList

    if frameid in _filterList[busid]:
        del _filterList[busid][frameid]

def _disconnect_client():
    global shutdownFlag
    global connected_client_addr
    global clientVersion
    global clientConnected
    global clientLastSeen

    logging.info("Disconnecting Client...")

    clientConnected = False
    clientLastSeen = 0
    clientVersion = 0

    connected_client_addr = 0

    logging.info("Disconnect Complete")

def serverListenerThread(name):
    global shutdownFlag
    global connected_client_addr
    global clientVersion
    global clientConnected
    global udp_socket
    global v2SendAll
    global _filterList


    logging.info("Waiting for client to connect...")

    while not shutdownFlag:
        try:
            data,recv_client_addr = udp_socket.recvfrom(1024)
        except socket.error:
            time.sleep(0.05)
        else:
            receivedLength = len(data)

            is_new_connection = False
            is_different_connection = False
            is_existing_connection = False
            if connected_client_addr == None:
                # We don't have a connection.  Treat this as new
                is_new_connection = True
            else:
                # We have an existing connection.
                # Is the same client sending us data
                if recv_client_addr != connected_client_addr:
                    # This is a different connection
                    is_different_connection = True
                else:
                    is_existing_connection = True

            # Default to the original protocol version
            protocolVersion = 1

            handle_connect = False
            handle_disconnect = False

            if data[0] == ord('H') or data[0] == ord('h'):
                # should be a standard panda "hello" packet
                if receivedLength == 5:
                    if data.decode().lower() == "hello":
                        # Standard panda hello packet
                        if is_new_connection:
                            handle_connect = True
                        elif is_existing_connection:
                            # Nothing to do but heartbeat
                            pass
                        elif is_different_connection:
                            handle_disconnect = True
                            handle_connect = True
                    else:
                        # This was data that was the right length, but didn't match the standard packet format (expecting "hello")
                        # We just assume this is still a heartbeat and move on with life
                        pass
                else:
                    # The data wasn't the right length to be a panda "hello" packet.  But it started with a 'H'...  Strange, but lets just
                    # assume that this is still some kinda heartbeat from the client and move on with life.
                    pass
            elif data[0] == ord('E') or data[0] == ord('e'):
                # Checking for the evolved panda protocol packet
                if receivedLength == 5:
                    if data.decode().lower() == "ehllo":
                        # We have an evolved panda protocol packet
                        protocolVersion = 2
                        if is_new_connection:
                            handle_connect = True
                        elif is_existing_connection:
                            # Nothing to do but heartbeat
                            pass
                        elif is_different_connection:
                            handle_disconnect = True
                            handle_connect = True
                    else:
                        # This was data that was the right length, but didn't match the evolved packet format (expecting "ehllo")
                        # We just assume this is an older packet version
                        pass
                else:
                    # The data wasn't the right length to be an evolved panda "ehllo" packet.  But it started with a E'...  Strange, but lets just
                    # assume that this is still some kinda heartbeat/connect from the client and move on with life.
                    pass
            elif data[0] == ord('B') or data[0] == ord('b'):
                # Check to see if this is a "bye" packet from the client
                if receivedLength == 3:
                    if data.decode().lower() == "bye":
                        # We have an evolved panda protocol disconnect
                        protocolVersion = 2
                        if is_new_connection:
                            # We don't do anything here.  Only disconnect an existing connection
                            pass
                        elif is_existing_connection:
                            handle_disconnect = True
                    else:
                        # This was data that was the right length, but didn't match the standard packet format (expecting "bye")
                        # We just assume this is still a heartbeat and move on with life
                        pass
                else:
                    # The data wasn't the right length to be a panda "bye" packet.  But it started with a 'B'...  Strange, but lets just
                    # assume that this is still some kinda heartbeat/connect from the client and move on with life.
                    pass

            if handle_disconnect:
                # Disconnect the existing client.
                _disconnect_client()

            if handle_connect:
                logging.info("Client Connected: %s:%d (ver: %d)", recv_client_addr[0], recv_client_addr[1], protocolVersion)

                # We copy the client's address struct here.  This only happens on a connect.
                # So even if we receive udp packets from another client all it will do is keep the heartbeat going.
                connected_client_addr = recv_client_addr
                clientVersion = protocolVersion


                # We send out a special bus 15 0x006 frame to tell the client that we are good to go in v2 mode
                ack_data = struct.pack("<IIQ", 0x006 << 21, (0 & 0x0F) | (15 << 4), 0)
                udp_socket.sendto(ack_data, connected_client_addr)

                #// Flag that there is a client connected.
                clientConnected = True


            if clientConnected:
                clientLastSeen = time.time()

                if data[0] == 0x0F and clientVersion == 2:      # Shift IN character
                    filterCount = int((receivedLength-1) / 3)

                    #logging.debug(" + Processing %d", filterCount)

                    for i in range(0, filterCount):
                        busid = data[(i*3) + 1]
                        frameid = (data[((i*3) + 2) + 1] & 0xFF) + (data[((i*3) + 1) + 1] << 8)

                        logging.info ("+ BusId: %d, FrameId: 0x%03X", busid, frameid)
                        if busid == -1:
                             _addFilterForBusAndFrameId(0, frameid)
                             _addFilterForBusAndFrameId(1, frameid)
                        else:
                            _addFilterForBusAndFrameId(busid, frameid)

                elif data[0] == 0x0E and clientVersion == 2:     # Shift OUT character
                    filterCount = int((receivedLength-1) / 3)

                    #logging.debug(" - Processing %d", filterCount)

                    for i in range(0, filterCount):
                        busid = data[(i*3) + 1]
                        frameid = (data[((i*3) + 2) + 1] & 0xFF) + (data[((i*3) + 1) + 1] << 8)

                        logging.info ("- BusId: %d, FrameId: 0x%03X", busid, frameid)

                        if busid == -1:
                            _removeFilterForBusAndFrameId(0, frameid)
                            _removeFilterForBusAndFrameId(1, frameid)
                        else:
                            _removeFilterForBusAndFrameId(busid, frameid)


                elif data[0] == 0x18 and clientVersion == 2:     # Cancel character
                    logging.debug("Clear Filters")
                    v2SendAll = False
                    _filterList = [{}, {}]

                elif data[0] == 0x0C and clientVersion == 2:     # Form Feed character
                    logging.debug("Send all frames")
                    v2SendAll = True

        if clientConnected:
            if time.time() - clientLastSeen > 10:
                # Client is no longer around (didn't heartbeat quickly enough.)  Bye bye.
                logging.info("Client timed out")
                _disconnect_client()

    logging.info("Server shutting down")

def dataSenderThread(name):
    global shutdownFlag
    global clientConnected
    global clientVersion
    global connected_client_addr
    global udp_socket
    global logreader
    global v2SendAll
    global _filterList

    lastClientConnected = False

    while not shutdownFlag:
        currentClientConncected = clientConnected

        if currentClientConncected:

            reopenLogs = False
            if currentClientConncected != lastClientConnected:
                logging.debug("Client has connected.  Preparing logs for sending")
                #A client just connected.  Reset the log file reader back to the start and begin sending data.
                reopenLogs = True

            if reopenLogs:
                logreader.close()
                logreader.addFile(sys.argv[1])
                logreader.begin()

            lastframetime = 0
            for frame in logreader.nextFrame():
                if currentClientConncected:
                    if lastframetime == 0:
                        lastframetime = frame.time

                    frametimediff = (frame.time - lastframetime) * 0.000001

                    if frametimediff > 0:
                        time.sleep(frametimediff)

                    #send this frame out the udp socket
                    send_frame = False

                    if clientVersion == 2:
                        if frame.busid == 15:
                            # always send magic bus frames
                            send_frame = True
                        elif v2SendAll:
                            send_frame = True
                        else:
                            if frame.id in _filterList[frame.busid]:
                                send_frame = True
                    else:
                        # v1 sends all frames
                        send_frame = True


                    if send_frame and clientConnected:
                        frame_data = struct.pack("<II", frame.id << 21, (frame.length & 0x0F) | (frame.busid << 4))
                        frame_data += struct.pack("<%dB" % frame.length, *frame.payload)
                        udp_socket.sendto(frame_data, connected_client_addr)

                    lastframetime = frame.time
                else:
                    break

        else:
            time.sleep(0.1)

        lastClientConnected = currentClientConncected


if (logreader.begin()):
    print("Using log file: " + sys.argv[1])

    server_address = ("0.0.0.0", 1338)
    udp_socket.bind(server_address)

    x = threading.Thread(target=serverListenerThread, args=(1,))
    x.start()

    y = threading.Thread(target=dataSenderThread, args=(1,))
    y.start()

    try:
        while True:
            time.sleep(0.1)
    except:
        pass

    shutdownFlag = True
    y.join()
    x.join()

else:
    print("Error loading log file: " + sys.argv[1])
    exit(1)

