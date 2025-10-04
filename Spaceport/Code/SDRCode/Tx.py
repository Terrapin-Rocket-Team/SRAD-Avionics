import numpy as np
import matplotlib.pyplot as plt
#from hackrf import HackRF
from Bitdelay import bitDelayed
from ConvertToBinary import ConvertToBinary

# CENTER_FREQ = 1250000000    # Hz
# L = 24                      # oversampling factor,L=Tb/Ts(Tb=bit period,Ts=sampling period)
# SAMPLE_RATE = # bit / sec

class Tx:

    def __init__(self, data = "frequency"):
        tempObj = ConvertToBinary()
        self.data_str_8b = tempObj.strToBinary(data)[0]
        self.imaginaryForm_np_array = []

    def modulate(self, sampleRate = 10**6):

        phaseShift = 0

        for bit in range(len(self.data_str_8b)):
            if self.data_str_8b[bit] == "0":
                phaseShift = 0
            elif self.data_str_8b[bit] == "1":
                phaseShift = np.pi
        
            self.imaginaryForm_np_array.append(np.exp(
                1j *
                ((2*np.pi) *
                (433 * (10**6)) * 
                ( (np.array(range( int((bit / 10**4 * 10**6)), int(((bit+1) / 10**4 * 10**6)) ))) / (10**6))  + phaseShift)
            ))

test = Tx()
test.modulate()
print(test.imaginaryForm_np_array)
x = [ele.real for ele in test.imaginaryForm_np_array]
y = [ele.imag for ele in test.imaginaryForm_np_array]

plt.scatter(x, y)
plt.xlim(-2, 2)
plt.ylim(-0.001, 0.001)
plt.ylabel('Imaginary')
plt.xlabel('Real')
plt.show()