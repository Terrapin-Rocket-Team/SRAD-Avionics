import pyserial as serial
import time

class FCtoHackerF: 

    # for UART, port = ? 
    def transmit_data(self, port='/dev/ttyS0'):
        # send "ping" to flight controller
        ser.write(b'ping\n')

        # read pong
        data = ser.read(4).decode('utf-8')

        while True: 
            try:
                with serial.Serial(port) as ser:
                    if ser.in_waiting>0:
                        data = ser.read(ser.in_waiting).decode('utf-8')
            except Exception as e:
                print(f"Error reading from flight controller: {e}")
            
                    
            # # send data to HackRF
            # if data:
            #     try:
            #         hackrf = HackRF()
            #         hackrf.setup()
            #         hackrf.start_tx_mode()
            #         hackrf.send(data)
            #         hackrf.stop_tx_mode()
            #         hackrf.close()
            #     except Exception as e:
            #         print(f"Error sending data to HackRF: {e}")

            # Write data to log files
            for string in data.split('\n'):
                if string[0:3] == "LOG":
                    with open("log.txt", "a") as log_file:
                        log_file.write(string+"\n")
                else:
                    with open ("telem.txt", "a") as telem_file:
                        telem_file.write(string+"\n")

            # FC sends at 50hz
            time.sleep(0.02)
