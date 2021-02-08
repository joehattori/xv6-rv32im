#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

#define PCI_ECAM        0x30000000
#define E1000_ID        0x100e8086
#define E1000_REGS_ADDR 0x40000000

void
pci_init()
{ 
  // using PCIe ECAM.
  uint32 *ecam = (uint32*) PCI_ECAM;
  for (int dev = 0; dev < 32; dev++) {
    int bus = 0;
    int func = 0;
    int offset = 0;
    uint32 diff = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    volatile uint32 *base = ecam + diff;
    if (*base == E1000_ID) {
      printf("E1000 found\n");
      // command register
      *(base + 1) = 0x7;
      // set base address
      *(base + 4) = E1000_REGS_ADDR;
      e1000_init((uint32*) E1000_REGS_ADDR);
    }
  }
}
