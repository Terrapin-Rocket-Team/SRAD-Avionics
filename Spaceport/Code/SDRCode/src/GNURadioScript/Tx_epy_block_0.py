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

    def __init__(self, port = '/dev/ttyAMA10'):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Embedded Python Block',   # will show up in GRC
            in_sig=[],
            out_sig=[np.uint8]
        )

        
        # if an attribute with the same name as a parameter is found,
        # a callback is registered (properties work, too).
        #instance field port val
        self.port = port
        self.serial = serial.Serial(self.port)


    def work(self, input_items, output_items):
# send "ping" to flight controller
        self.serial.write(b'ping\n')

        data = self.serial.read(4).decode('utf-8')
        print(data)

        # # creates a serial object to read data from flight computer
        # for i in range(9,000):
        #     try:
        #         if self.serial.in_waiting>0:
        #             data = self.serial.read(self.serial.in_waiting).decode('utf-8')
        #             return bytes(data)
        #     except Exception as e:
        #         print(f"Error reading from flight controller: {e}")

        #     # still in our continous loop, write the data 
        #     for string in data.split('\n'):
        #         if string[0:3] == "LOG": #change to cooresponding data down the line
        #             with open("log.txt", "a") as log_file:
        #                 log_file.write(string+"\n")
        #         else:
        #             with open("telem.txt", "a") as telem_file:
        #                 telem_file.write(string+"\n")

        #     # determine the rate of sending
        #     time.sleep(0.02) #50 hZ

    # return len(output_items[0])
