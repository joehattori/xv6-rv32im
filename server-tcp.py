import socket
import sys

addr = ('localhost', int(sys.argv[1]))
print(f'listening on port {addr}')

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind(addr)
    s.listen()
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while True:
            data = conn.recv(1024)
            print(data)
            if not data:
                break
            conn.sendall(data)
