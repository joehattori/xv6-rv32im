#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define E1000_IMS   0x0d0
#define E1000_TCTL  0x400
#define E1000_TDBAL 0x3800
#define E1000_TDBAH 0x3804
#define E1000_TDLEN 0x3808
#define E1000_TDH   0x3810
#define E1000_TDT   0x3818

#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

struct TxDescriptor {
  uint64 buf_addr;
  uint64 fields;
};

struct RxDescriptor {
  uint64 buf_addr;
  uint64 fields;
};

static struct TxDescriptor tx_ring_buffer[TX_RING_SIZE] __attribute__((aligned(16)));
// static struct RxDescriptor rx_ring_buffer[RX_RING_SIZE] __attribute__((aligned(16)));

static volatile uint32 regs_addr;

static void
set_e1000_regs(uint32 offset, uint32 data)
{
  *(uint32*)(regs_addr + offset) = data;
}

static uint tx_current_idx;

static void
e1000_tx_init()
{
  tx_current_idx = 0;

  set_e1000_regs(E1000_TDBAL, (uint32)tx_ring_buffer);
  set_e1000_regs(E1000_TDLEN, TX_RING_SIZE);
  set_e1000_regs(E1000_TDH, tx_current_idx);
  set_e1000_regs(E1000_TDT, tx_current_idx);
  set_e1000_regs(E1000_TCTL, (0x40 << 12) | (0xf << 4) | (1 << 3) | (1 << 1));

  // init tx descriptors.
  // set RS (bit 3) and EOP (bit 0) to 1.
  uint cmd = 0b1001;
  for (int i = 0; i < TX_RING_SIZE; i++)
    tx_ring_buffer[i].fields = cmd << 24;
}

static struct spinlock e1000_lock;

void
e1000_init(uint32 *e1000_regs)
{
  initlock(&e1000_lock, "e1000");
  regs_addr = (uint32)e1000_regs;
  // disable interruption.
  set_e1000_regs(E1000_IMS, 0);
  e1000_tx_init();
}

int
e1000_send_frame(void *buf, ushort len)
{
  acquire(&e1000_lock);

  struct TxDescriptor *tail = &tx_ring_buffer[tx_current_idx];
  
  // clear STA field and set length.
  tail->fields = (tail->fields & 0xfffffff0ffffffff) | (uint16) len;
  tail->buf_addr = (uint64)(uint32) buf;

  tx_current_idx = (tx_current_idx + 1) % TX_RING_SIZE;
  set_e1000_regs(E1000_TDT, (tx_current_idx + 1) % TX_RING_SIZE);

  // busy wait till DD bit in status field is 1.
  while (!((tail->fields >> 32) & 1));

  release(&e1000_lock);
  return 0;
}
