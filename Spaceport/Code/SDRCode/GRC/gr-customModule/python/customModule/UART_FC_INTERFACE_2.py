#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2025 Louie.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import numpy as np
from gnuradio import gr 
import serial
import time

class UART_FC_INTERFACE_2(gr.sync_block):
    """
    docstring for block UART_FC_INTERFACE_2
    """
    def __init__(self, port = '/dev/ttyS0'):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.sync_block.__init__(
            self,
            name='Embedded Python Block',   # will show up in GRC
            in_sig=[],
            out_sig=[np.uint8]
        )
        
        self.port = port

    def work(self, output_items):
        # send "ping" to flight controller
        ser.write(b'ping\n')

        data = ser.read(4).decode('utf-8')

        # creates a serial object to read data from flight computer
        while True:
            try:
                with serial.Serial(self.port) as ser:
                    if ser.in_waiting>0:
                          data = ser.read(ser.in_waiting).decode('utf-8')
                          return bytes(data)
            except Exception as e:
                print(f"Error reading from flight controller: {e}")

            # still in our continous loop, write the data 
            for string in data.split('\n'):
                if string[0:3] == "LOG": #change to cooresponding data down the line
                    with open("log.txt", "a") as log_file:
                        log_file.write(string+"\n")
                else:
                    with open("telem.txt", "a") as telem_file:
                        telem_file.write(string+"\n")

            # determine the rate of sending
            time.sleep(0.02) #50 hZ
