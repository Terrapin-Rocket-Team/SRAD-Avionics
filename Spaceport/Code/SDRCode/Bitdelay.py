from ConvertToBinary import ConvertToBinary

# takes a string, changes it to binary and encodes it as described here 
# under differential coding: https://pysdr.org/content/digital_modulation.html
def bitDelayed(strToChange):
    final_binary_str, final_binary_arr = ConvertToBinary().strToBinary(strToChange)
    other_bit = 1
    bit_delayed_str = ""

    for i in range(len(final_binary_str)):
        next_bit = int(final_binary_str[i]) ^ other_bit
        bit_delayed_str += next_bit
        other_bit = next_bit

    return bit_delayed_str



