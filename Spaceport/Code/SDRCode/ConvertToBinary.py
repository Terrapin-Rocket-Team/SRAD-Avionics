class ConvertToBinary:

    def encodeString(self, stringToEncode):
        stringToEncode = "0"+stringToEncode

        encoded_str = stringToEncode

        for bit in range(1, len(encoded_str)):

            if encoded_str[bit] == encoded_str[bit-1]:
                encoded_str = encoded_str[0:bit]+"0"+encoded_str[bit+1:]
            else:
                encoded_str = encoded_str[0:bit]+"1"+encoded_str[bit+1:]

        return encoded_str

    def strToBinary(self, stringToChange):
        finalBinary_str = ""
        finalBinary_arr = []
        
        for character in stringToChange:
            char_b = bin(ord(character))[2:]
            if (len(char_b) < 8):
                num_zeros = 8 - len(char_b)
                char_b = ("0"*num_zeros)+char_b
            
            finalBinary_arr.append(char_b)
            finalBinary_str += char_b
            
        # differential encoding for raw data
        return finalBinary_str, finalBinary_arr
    
    def getDataFromPi(self):
        pass
    
print(ConvertToBinary().encodeString("11010"))