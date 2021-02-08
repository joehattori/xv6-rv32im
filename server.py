import socket
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
addr = ('localhost', int(sys.argv[1]))
print(f'listening on port {addr}')
sock.bind(addr)

while True:
    buf, raddr = sock.recvfrom(4096)
    print(buf)
    if buf:
        sent = sock.sendto(buf, raddr)
