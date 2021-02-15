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
