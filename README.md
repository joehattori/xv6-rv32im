# What is this?
This is a ported version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv/) to rv32im, equipped with some network stacks.
Ethernet, IP, ARP, DNS resolving, UDP, TCP, and HTTP is (partly and roughly) implemented.
TCP implementation is based on https://github.com/pandax381/microps.

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
