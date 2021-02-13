# What is this?
This is a ported version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv/) to rv32im, equipped with some network stacks.
Ethernet, IP, ARP, DNS resolving, TCP and UDP is (partly and roughly) implemented.

# How to start shell on qemu
After installing some RISC-V tools,
```bash
$ make qemu
```
should start the shell for xv6 in qemu environment.
If you want to use docker,
```bash
$ make run-docker
```
will pull a docker image with necessary tools and starts a container.
