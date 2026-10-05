import re
import os
import sys

regName = ""

if not sys.argv[1]:
    print("Error: must specify register name")
else:
    regName = sys.argv[1]

reg_settings = os.listdir("reg_values")

o = open("out.csv", "w")

firstFile = True
for file in reg_settings:
    # open file
    f = open(os.path.join("reg_values", file), "r")
    text = ""

    # read entire file
    line = f.readline()
    while (line):
        text += line
        line = f.readline()

    # find desired property
    m = re.search(r"/\*[^#]+#define " + regName + r"( 0x[0-9ABCDEFabcdef]+,?)+", text)
    if (m):
        prop = m.group()
        # print("Found:\n")
        # print(prop)
        if firstFile:
            mDesc = re.search(r"(?<=// Descriptions:\n)[^*]+", prop)
            names = re.findall("(?<=//   )[0-9A-z]+(?= )", mDesc.group())
            o.write("Setting: " + regName + "," + ",".join(names) + "\n")
            firstFile = False
        mVal = re.search(r"(?<=#define " + regName + r" ).+", prop)
        vals = mVal.group().split(", ")
        del vals[0:4]
        o.write(file.split(".")[0] + "," + ",".join(vals) + "\n")
    else:
        print("Error: could not find property match")


    
