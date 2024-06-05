video1 = "./inputs/vid1.av1"
video2 = "./inputs/vid2.av1"

output_dest = "./output/mux.txt"

source1_size = 1250
source2_size = 1250

telemetry_string = "telemetry"

# Read the video files and write out a text file of the two videos and a constant string multiplexed together
# The output file will be used as input to the demuxer
# The format is the number 1, followed by the size of source 1 in the next 2 bytes, followed by the specific bytes of source 1.
# The same is done for source 2. This is repeated 15 times, alternating between the two sources. Then, the number 3 is written, 
# followed by the size of the constant string, followed by the constant string. This pattern is repeated until the end of the file.
def write_mux():
    with open(video1, "rb") as f1, open(video2, "rb") as f2, open(output_dest, "wb") as out:
        for i in range(15):
            out.write(b'\x01')
            out.write(source1_size.to_bytes(2, byteorder='big'))
            out.write(f1.read(source1_size))
            out.write(b'\x02')
            out.write(source2_size.to_bytes(2, byteorder='big'))
            out.write(f2.read(source2_size))
        out.write(b'\x03')
        out.write(len(telemetry_string).to_bytes(2, byteorder='big'))
        out.write(telemetry_string.encode('utf-8'))

if __name__ == "__main__":
    write_mux()