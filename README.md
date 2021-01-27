# What is this?
This is a ported version of [xv6-riscv](https://github.com/mit-pdos/xv6-riscv/) to rv32im.

# How to start shell on qemu
First, run
```bash
$ make run-docker
$ make # inside the launched docker container
$ make fs.img # inside the launched docker container
```
then run
```bash
$ make qemu # outside container
```
This should start the shell for xv6.
`qemu-system-riscv32` is required.
