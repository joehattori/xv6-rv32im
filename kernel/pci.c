#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

#define E1000_ID 0x100e8086

void
pci_init()
{ 
  // set e1000 registers to this address.
  // uint32 e1000_regs = 0x40000000;
  // using PCIe ECAM.
  uint32 *ecam = (uint32*)0x30000000L;
  for (int dev = 0; dev < 32; dev++) {
    int bus = 0;
    int func = 0;
    int offset = 0;
    uint32 diff = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    volatile uint32 *base = ecam + diff;
    if (base[0] == E1000_ID) {
      printf("E1000 found\n");
    }
  }
}
