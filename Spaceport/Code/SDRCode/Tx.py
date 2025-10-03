import numpy as np
#from hackrf import HackRF
from ConvertToBinary import ConvertToBinary

# CENTER_FREQ = 1250000000    # Hz
# L = 24                      # oversampling factor,L=Tb/Ts(Tb=bit period,Ts=sampling period)
# SAMPLE_RATE = # bit / sec

class Tx:

    def __init__(self, data):
        self.data_str_8b, self.data_arr_8b = ConvertToBinary().strToBinary(data)
        self.imaginaryForm = np.array([])

    def modulate(self, sampleRate):
        for bit in self.data_str_8b:
            if bit == 0:
                pass
            elif bit == 1:
                pass

test = Tx("asdfsdfds")