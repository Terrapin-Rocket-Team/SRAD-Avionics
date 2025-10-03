import numpy as np
import matplotlib.pyplot as plt
#from hackrf import HackRF
from Bitdelay import bitDelayed
from ConvertToBinary import ConvertToBinary

# CENTER_FREQ = 1250000000    # Hz
# L = 24                      # oversampling factor,L=Tb/Ts(Tb=bit period,Ts=sampling period)
# SAMPLE_RATE = # bit / sec

class Tx:

    def __init__(self, data):
        tempObj = ConvertToBinary()
        self.data_str_8b = tempObj.strToBinary(data)
        self.imaginaryForm_np_array = []

    def modulate(self, sampleRate = 10**6):

        phaseShift = 0

        for bit in self.data_str_8b:
            if bit == 0:
                phaseShift = 0
            elif bit == 1:
                phaseShift = np.pi
            
            print(np.exp(
                1j *
                (2*np.pi) *
                (433 * (10**6)) * 
                ((len(self.data_str_8b)) / sampleRate) + phaseShift
            ))

test = Tx("0101")
test.modulate()