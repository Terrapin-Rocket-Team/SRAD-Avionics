import numpy as np
#from hackrf import HackRF
from Bitdelay import bitDelayed

# CENTER_FREQ = 1250000000    # Hz
# L = 24                      # oversampling factor,L=Tb/Ts(Tb=bit period,Ts=sampling period)
# SAMPLE_RATE = # bit / sec

class Tx:

    def __init__(self, data, sampleRate):
        self.data_str_8b = bitDelayed(data)
        self.imaginaryForm_np_array = np.array([])
        self

    def modulate(self, sampleRate):
        for bit in self.data_str_8b:
            if bit == 0:
                pass
            elif bit == 1:
                pass

test = Tx("asdfsdfds")