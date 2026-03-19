#!/usr/bin/env python3


#this will be used for connecting via wifi and tcp client stuff
#must be bidirectional transport bridge 
SERVER_HOST     = "arcpi8.local" #IP of the other Pi Zero (TCP server) MUST CHANGE 
SERVER_PORT     = 5000             # Must match server
SERIAL_PORT = "/dev/serial0"   # serial address MUST CHANGE
SERIAL_BAUD = 115200           # baud rate
RECONNECT_DELAY = 0.2                # Seconds between WiFi reconnect attempts
BUFFER_SIZE     = 4096
import socket
import threading
import time
import sys
import logging
import serial  

def serial_to_wifi(ser: serial.Serial, sock: socket.socket, stop_event: threading.Event) -> None:
    while not stop_event.is_set():
        try:
            data = ser.read(ser.in_waiting)
            #data = b'Hello world'
            if data:
                sock.sendall(data)
                #time.sleep(1)
            else:
                time.sleep(0.005)
        except (serial.SerialException, OSError):
            stop_event.set()
            break

def wifi_to_serial(sock: socket.socket, ser: serial.Serial, stop_event: threading.Event) -> None:
    sock.settimeout(None)
    while not stop_event.is_set():
        try:
            data = sock.recv(BUFFER_SIZE)
            if not data:
                print("drop")
                stop_event.set()
                break
            print(data.decode())
            ser.write(data)
        except socket.timeout:
            print("timeout")
            continue
        except (OSError, serial.SerialException):
            stop_event.set()
            break

def main():
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0)
 
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)
            sock.connect((SERVER_HOST, SERVER_PORT))

            stop_event = threading.Event()
 
            t1 = threading.Thread(target=serial_to_wifi, args=(ser, sock, stop_event), daemon=True)
            t2 = threading.Thread(target=wifi_to_serial, args=(sock, ser, stop_event), daemon=True)
 
            t1.start()
            t2.start()
 
            stop_event.wait()
 
        except (ConnectionRefusedError, OSError, socket.timeout):
            pass
        finally:
            try:
                sock.close()
            except Exception:
                pass
 
        time.sleep(RECONNECT_DELAY)
 
if __name__ == "__main__":
    main()
