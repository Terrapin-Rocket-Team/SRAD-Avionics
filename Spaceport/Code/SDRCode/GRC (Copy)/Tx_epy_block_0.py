"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr 
import serial
import time


class blk(gr.sync_block):  # other base classes are basic_block, decim_block, interp_block
    """Embedded Python Block example - a simple multiply const"""

    def __init__(self, port = '/dev/ttyS0'):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Embedded Python Block',   # will show up in GRC
            in_sig=[np.complex64],
            out_sig=[np.complex64]
        )

        
        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
        #instance field port val
        self.port = port


    def work(self, input_items, output_items):
# send "ping" to flight controller
        ser.write(b'ping\n')

        data = ser.read(4).decode('utf-8')

        # creates a serial object to read data from flight computer
        while True:
            try:
                with serial.Serial(self.port) as ser:
                    if ser.in_waiting>0:
                        data = ser.read(ser.in_waiting).decode('utf-8')
            except Exception as e:
                print(f"Error reading from flight controller: {e}")

            # still in our continous loop, write the data 
            for string in data.split('\n'):
                if string[0:3] == "LOG": #change to cooresponding data down the line
                    with open("log.txt", "a") as log_file:
                        log_file.write(string+"\n")
                else:
                    with open("telem.txt", "a") as telem_file:
                        telem_file.write("test")

            # determine the rate of sending
            time.sleep(0.02) #50 hZ

    # return len(output_items[0])
