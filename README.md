# What is this?
This is a ported version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv/) to rv32im, equipped with some network stacks.
Ethernet, IP, ARP, DNS resolving, UDP, TCP, and HTTP is (partly and roughly) implemented.
TCP implementation is based on https://github.com/pandax381/microps.
Also using [This repo by MIT](https://github.com/mit-pdos/xv6-riscv-fall19/tree/net) as a reference.

# How to start shell on qemu
After installing some RISC-V tools,
```bash
$ make qemu
```
should start the shell for xv6 in qemu environment.
Otherwise,
```bash
$ make run-docker
```
will pull a docker image with necessary tools and starts a container.

# curl
`curl`-like program will run! It prints the response of the GET request to the specified url.
```bash
$ make qemu
# in qemu environment
$ curl example.com
HTTP/1.1 200 OK
Accept-Ranges: bytes
Age: 575273
Cache-Control: max-age=604800
Content-Type: text/html; charset=UTF-8
Date: Wed, 03 Mar 2021 01:19:10 GMT
Etag: "3147526947"
Expires: Wed, 10 Mar 2021 01:19:10 GMT
Last-Modified: Thu, 17 Oct 2019 07:18:26 GMT
Server: ECS (sjc/4E73)
Vary: Accept-Encoding
X-Cache: HIT
Content-Length: 1256

<!doctype html>
<html>
<head>
    <title>Example Domain</title>
...
```
Currently HTTPS is not supported.
