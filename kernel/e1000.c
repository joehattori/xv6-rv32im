#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net.h"

#define E1000_IMS   0x0d0
#define E1000_ICR   0x0d8
#define E1000_RDBAL 0x2800
#define E1000_RDLEN 0x2808
#define E1000_RDH   0x2810
#define E1000_RDT   0x2818
#define E1000_RCTL  0x100
#define E1000_RAL   0x5400
#define E1000_RAH   0x5404
#define E1000_TDBAL 0x3800
#define E1000_TDLEN 0x3808
#define E1000_TDH   0x3810
#define E1000_TDT   0x3818
#define E1000_TCTL  0x400

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
static struct RxDescriptor rx_ring_buffer[RX_RING_SIZE] __attribute__((aligned(16)));

static struct mbuf *tx_mbufs[TX_RING_SIZE];
static struct mbuf *rx_mbufs[RX_RING_SIZE];

static volatile uint32 regs_addr;

static void
set_e1000_regs(uint32 offset, uint32 data)
{
  *(uint32*)(regs_addr + offset) = data;
}

static volatile uint tx_current_idx, rx_current_idx;

static void
e1000_tx_init()
{
  tx_current_idx = 0;

  set_e1000_regs(E1000_TDBAL, (uint32)tx_ring_buffer);
  set_e1000_regs(E1000_TDLEN, TX_RING_SIZE);
  set_e1000_regs(E1000_TDH, tx_current_idx);
  set_e1000_regs(E1000_TDT, tx_current_idx);
  set_e1000_regs(E1000_TCTL, (0x40 << 12) | (0xf << 4) | (1 << 3) | (1 << 1));

  // set RS (bit 3) and EOP (bit 0) to 1.
  uint cmd = 0b1001;
  for (int i = 0; i < TX_RING_SIZE; i++) {
    tx_ring_buffer[i].fields = cmd << 24;
    tx_mbufs[i] = mbuf_alloc(0);
  }
}

static void
e1000_rx_init()
{
  rx_current_idx = 0;

  set_e1000_regs(E1000_RDBAL, (uint32)rx_ring_buffer);
  set_e1000_regs(E1000_RDLEN, RX_RING_SIZE);
  set_e1000_regs(E1000_RDH, 0);
  set_e1000_regs(E1000_RDT, RX_RING_SIZE - 1);
  set_e1000_regs(E1000_RCTL, (1 << 15) | 0b11110);

  for (int i = 0; i < RX_RING_SIZE; i++) {
    rx_mbufs[i] = mbuf_alloc(0);
    rx_ring_buffer[i].fields = 0;
    rx_ring_buffer[i].buf_addr = (uint64) (uint32) rx_mbufs[i]->head;
  }

  // qemu's MAC address is 52:56:00:12:34:56. store it backwards.
  set_e1000_regs(E1000_RAL, 0x12005452);
  set_e1000_regs(E1000_RAH, (1 << 31) | 0x5634);

  // mask for receive timer interrupt
  set_e1000_regs(E1000_IMS, 1 << 7);
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
  e1000_rx_init();
}

int
e1000_send(struct mbuf *m)
{
  acquire(&e1000_lock);

  struct TxDescriptor *tail = &tx_ring_buffer[tx_current_idx];
  
  // clear STA field and set length.
  tail->fields = (tail->fields & 0xfffffff0ffffffff) | (uint16) m->len;
  tail->buf_addr = (uint64)(uint32) m->head;

  if (tx_mbufs[tx_current_idx])
    mbuf_free(tx_mbufs[tx_current_idx]);
  tx_mbufs[tx_current_idx] = m;

  set_e1000_regs(E1000_TDT, (tx_current_idx + 1) % TX_RING_SIZE);

  // busy wait till DD bit in status field is 1.
  while (!((tail->fields >> 32) & 1));

  release(&e1000_lock);
  return 0;
}

static void
e1000_recv(void)
{
  uint nxt_idx = (rx_current_idx + 1) % RX_RING_SIZE;
  // process the packet if the DD bit in status field is non-zero
  while ((rx_ring_buffer[nxt_idx].fields >> 32) & 1) {
    acquire(&e1000_lock);
    struct mbuf *buf = rx_mbufs[nxt_idx];
    mbuf_append(buf, rx_ring_buffer[nxt_idx].fields & 0xff);
    set_e1000_regs(E1000_RDT, nxt_idx);

    rx_mbufs[nxt_idx] = mbuf_alloc(0);
    if (!rx_mbufs[nxt_idx])
      panic("e1000 recv");

    // clear the STA field.
    rx_ring_buffer[nxt_idx].fields &= 0xfffffff0ffffffff;
    rx_ring_buffer[nxt_idx].buf_addr = (uint64)(uint32) rx_mbufs[nxt_idx]->head;

    release(&e1000_lock);

    net_rx(buf);

    rx_current_idx = nxt_idx;
    set_e1000_regs(E1000_RDT, rx_current_idx);
  }
}

void
e1000_intr(void)
{
  e1000_recv();
  // read to ICR clears all the bits, which means to acknowledge any pending interrupt events.
  uint32 _ __attribute__((unused)) = *(uint32*)(regs_addr + E1000_ICR);
}
