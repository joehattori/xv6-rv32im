import socket
import sys
import time

addr = ('localhost', int(sys.argv[1]))
buf = b'this is a ping!'

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    print('pinging...')
    s.connect(addr)
    while True:
        s.sendto(buf, addr)
        time.sleep(1)

"""
0000   00 25 36 10 75 d9 a4 83 e7 3c 0b b9 08 00|45 00
0010   00 40 00 00 40 00 40 06 43 2c c0 a8 01 09 5d b8
0020   d8 22|cc 8c 00 50 eb e1 95 32 00 00 00 00 b0 02
0030   ff ff b3 50 00 00

0000   52 55 0a 00 02 02 52 54 00 12 34 56 08 00 45 00
0010   00 28 00 00 00 00 64 06 3e c0 0a 00 02 0f 0a 00
0020   02 02 07 d0 64 00 00 00 83 4d 00 00 00 00 50 02
0030   10 00 9f ad 00 00

"""
