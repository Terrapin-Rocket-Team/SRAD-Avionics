class ConvertToBinary:

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
            
        return finalBinary_str, finalBinary_arr
    
    def getDataFromPi(self):
        pass
