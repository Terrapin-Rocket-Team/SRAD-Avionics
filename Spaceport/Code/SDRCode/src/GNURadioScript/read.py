import serial
import time

for i in range (50 , 61):
    with serial.Serial(f'/dev/serial0', baudrate=115200) as ser:

        # if(i == )

        ser.write('ping\n'.encode())
        print(f'Sent {i}')
        resp = False
        count = 0
        while True:
            if ser.in_waiting > 0:
                a = ser.read(4)
                print(a)
                if(a == 'pong'):
                    resp = True
                    break
            time.sleep(.1)
            count += 1
            if(count > 3):
                break
            