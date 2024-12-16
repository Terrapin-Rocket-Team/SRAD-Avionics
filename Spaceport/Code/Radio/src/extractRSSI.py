from os import path
import re

folderPath = "logs"
filePath = "COM4_2024_10_20.11.59.04.194.txt"

outFile = "RSSI_Data.txt"

f = open(path.join(path.curdir, folderPath, filePath), "r")
o = open(path.join(path.curdir, outFile), "w")

line = f.readline()

while line:
    m = re.search("(?<=Signal strength: )-?[0-9]+", line)
    print(m)
    if m:
        o.write("%s\n" % line[m.start():m.end()])
    line = f.readline()

o.close()