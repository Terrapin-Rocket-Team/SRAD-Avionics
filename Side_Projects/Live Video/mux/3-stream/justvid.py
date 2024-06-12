video1 = "./inputs/vid1.av1"
output_dest = "./output/justvid.bin"

with open(video1, "rb") as f1, open(output_dest, "wb") as out:
    # write 3 bytes to the output file, then write the entire contents of the video file
    out.write(b'\x01')
    # write the number 65,535 in 2 bytes
    out.write((65535).to_bytes(2, byteorder='big'))
    f1.seek(0)

    out.write(f1.read())

