from struct import unpack
from struct import pack
import binascii
import sys

buf = bytearray(0)
with open('test.bin', 'rb') as ff:
    try:
        while True:
            rr = unpack('BB', ff.read(2))
            buf.append(rr[1])
            buf.append(rr[0])
    except Exception as e:
        print(str(e))
        print('finished!')
    else:
        pass

print(buf, len(buf))




