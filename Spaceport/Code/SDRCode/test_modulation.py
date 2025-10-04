import numpy as np
import matplotlib.pyplot as plt
#from hackrf import HackRF
from Bitdelay import bitDelayed
from ConvertToBinary import ConvertToBinary

CENTER_FREQUENCY = 434e6
SAMPLE_RATE = 1e6      # Samples per second (Hz)
BIT_RATE = 10e3        # Bits per second (Hz)
SAMPLES_PER_BIT = int(SAMPLE_RATE / BIT_RATE)


class Tx:

    def __init__(self, data = "frequency"):
        tempObj = ConvertToBinary()
        self.data_str_8b = tempObj.strToBinary(data)[0]
        self.imaginaryForm_np_array = []

    def modulate(self, sampleRate = 10**6):

        phaseShift = 0

        for bit in self.data_str_8b:
            if bit == "0":
                phaseShift = 0
            elif bit == "1":
                phaseShift = np.pi
        
            self.imaginaryForm_np_array.append(np.exp(
                1j *
                ((2*np.pi) *
                (433 * (10**6)) * 
                ((len(self.data_str_8b)) / sampleRate) + phaseShift)
            ))

test = Tx()
test.modulate()
print(test.imaginaryForm_np_array)
# x = [ele.real for ele in test.imaginaryForm_np_array]
# y = [ele.imag for ele in test.imaginaryForm_np_array]

# plt.scatter(x, y)
# plt.ylabel('Imaginary')
# plt.xlabel('Real')
# plt.show()