# What is this?
This is a ported version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv/) to rv32im.

# How to start shell on qemu
```bash
$ make run-docker
```
This will pull a docker image and starts a container. Within the container, run
```bash
$ make qemu
```
This should start the shell for xv6 in qemu environment.
