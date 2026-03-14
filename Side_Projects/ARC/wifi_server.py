#!/usr/bin/env python3
# TCP server side bidirectional serial, wifi bridge


SERVER_HOST     = "0.0.0.0"
SERVER_PORT     = 5000
SERIAL_PORT     = "/dev/ttyS0"  # MUST CHANGE
SERIAL_BAUD     = 115200
RECONNECT_DELAY = 0.2
BUFFER_SIZE     = 4096

import socket
import threading
import time
import serial



def serial_to_wifi(ser: serial.Serial, sock: socket.socket, stop_event: threading.Event) -> None:
    while not stop_event.is_set():
        try:
            waiting = ser.in_waiting
            if waiting:
                data = ser.read(waiting)
                sock.sendall(data)
            else:
                time.sleep(0.005)
        except (serial.SerialException, OSError):
            stop_event.set()
            break
            
            

def wifi_to_serial(conn: socket.socket, ser: serial.Serial, stop_event: threading.Event) -> None:
    conn.settimeout(1.0)
    while not stop_event.is_set():
        try:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                stop_event.set()
                break
            ser.write(data)
        except socket.timeout:
            continue
        except (OSError, serial.SerialException):
            stop_event.set()
            break

def main():
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((SERVER_HOST, SERVER_PORT))
        server.listen(1)
        print(f"Listening on port {SERVER_PORT}...")

        while True:
            conn, addr = server.accept()
            print(f"Connected: {addr}")

            stop_event = threading.Event()

            t1 = threading.Thread(target=serial_to_wifi, args=(ser, conn, stop_event), daemon=True)
            t2 = threading.Thread(target=wifi_to_serial, args=(conn, ser, stop_event), daemon=True)

            t1.start()
            t2.start()

            stop_event.wait()
            print("Client disconnected, waiting for new connection...")

            try:
                conn.close()
            except Exception:
                pass

if __name__ == "__main__":
    main()
