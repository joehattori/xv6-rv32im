#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "ethernet.h"
#include "mbuf.h"

#define E1000_CTL   (0x0000/4)
#define E1000_ICR   (0x00c0/4)
#define E1000_IMS   (0x00d0/4)
#define E1000_RCTL  (0x0100/4)
#define E1000_TCTL  (0x0400/4)
#define E1000_TIPG  (0x0410/4)
#define E1000_RDBAL (0x2800/4)
#define E1000_RDTR  (0x2820/4)
#define E1000_RADV  (0x282c/4)
#define E1000_RDH   (0x2810/4)
#define E1000_RDT   (0x2818/4)
#define E1000_RDLEN (0x2808/4)
#define E1000_TDBAL (0x3800/4)
#define E1000_TDLEN (0x3808/4)
#define E1000_TDH   (0x3810/4)
#define E1000_TDT   (0x3818/4)
#define E1000_MTA   (0x5200/4)
#define E1000_RAL   (0x5400/4)
#define E1000_RAH   (0x5404/4)

#define E1000_TXD_CMD_EOP  1
#define E1000_TXD_CMD_RS   8
#define E1000_TXD_STAT_DD  1
#define E1000_RXD_STAT_DD  1
#define E1000_RXD_STAT_EOP 2

struct tx_desc {
  uint64 addr;
  uint16 length;
  uint8 cso;
  uint8 cmd;
  uint8 status;
  uint8 css;
  uint16 special;
};

struct rx_desc {
  uint64 addr;
  uint16 length;
  uint16 csum;
  uint8 status;
  uint8 errors;
  uint16 special;
};

#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));

static struct mbuf *tx_mbufs[TX_RING_SIZE];
static struct mbuf *rx_mbufs[RX_RING_SIZE];

static volatile uint32 *regs;

struct spinlock e1000_lock;

static void
e1000_tx_init()
{
  uint32 tx_ring_size = sizeof(tx_ring);
  memset(tx_ring, 0, tx_ring_size);
  for (int i = 0; i < TX_RING_SIZE; i++) {
    tx_ring[i].status = E1000_TXD_STAT_DD;
    tx_mbufs[i] = 0;
  }
  regs[E1000_TDBAL] = (uint64)(uint32) tx_ring;
  regs[E1000_TDLEN] = tx_ring_size;
  regs[E1000_TDH] = regs[E1000_TDT] = 0;
}

static void
e1000_rx_init()
{
  uint32 rx_ring_size = sizeof(rx_ring);
  memset(rx_ring, 0, rx_ring_size);
  for (int i = 0; i < RX_RING_SIZE; i++) {
    rx_mbufs[i] = mbuf_alloc(0);
    if (!rx_mbufs[i])
      panic("e1000");
    rx_ring[i].addr = (uint64)(uint32) rx_mbufs[i]->head;
  }
  regs[E1000_RDBAL] = (uint64)(uint32) rx_ring;
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_RING_SIZE - 1;
  regs[E1000_RDLEN] = rx_ring_size;
}

void
e1000_init(uint32 *xregs)
{
  initlock(&e1000_lock, "e1000");

  regs = xregs;

  regs[E1000_IMS] = 0;

  e1000_tx_init();
  e1000_rx_init();
  
  // MAC address of qemu (52:54:00:12:34:56).
  regs[E1000_RAL] = 0x12005452;
  regs[E1000_RAH] = 0x5634 | (1<<31);
  for (int i = 0; i < 4096/32; i++)
    regs[E1000_MTA + i] = 0;
  regs[E1000_TCTL] = (0x40 << 12) | (0x10 << 4) | (1 << 3) | (1 << 1);
  regs[E1000_TIPG] = (6 << 20) | (8 << 10) | 10;
  regs[E1000_RCTL] = (0x4 << 24) | (0x8 << 12) | (1 << 1);
  regs[E1000_RDTR] = regs[E1000_RADV] = 0;
  regs[E1000_IMS] = (1 << 7);
}

int
e1000_send(struct mbuf *m)
{
  acquire(&e1000_lock);

  uint32 desc_pos = regs[E1000_TDT];
  
  struct tx_desc *tail = &tx_ring[desc_pos];

  if ((tail->status & E1000_RXD_STAT_DD) == 0) {
    release(&e1000_lock);
    printf("overflow\n");
    return -1;
  }
  
  if (tx_mbufs[desc_pos] != 0)
    mbuf_free(tx_mbufs[desc_pos]);

  tx_mbufs[desc_pos] = m;
  tail->addr = (uint64)(uint32) m->head;
  tail->length = m->len;
  tail->cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;

  regs[E1000_TDT] = (desc_pos + 1) % TX_RING_SIZE;
  __sync_synchronize();
  release(&e1000_lock);
  return 0;
}

static void
e1000_recv(void)
{
  uint32 desc_pos = (regs[E1000_RDT] + 1) % RX_RING_SIZE;

  while (1) {
    struct rx_desc *tail = &rx_ring[desc_pos];

    if (!(tail->status & E1000_RXD_STAT_DD))
      break;

    acquire(&e1000_lock);
    struct mbuf *buf = rx_mbufs[desc_pos];
    mbuf_append(buf, tail->length);

    rx_mbufs[desc_pos] = mbuf_alloc(0);
    if (!rx_mbufs[desc_pos])
      panic("e1000");
    tail->addr = (uint64)(uint32) rx_mbufs[desc_pos]->head;
    tail->status = 0;

    regs[E1000_RDT] = desc_pos;
    release(&e1000_lock);

    ethernet_rx(buf);

    desc_pos = (desc_pos + 1) % RX_RING_SIZE;
  }
}

void
e1000_intr(void)
{
  e1000_recv();

  // touch E1000_ICR to tell e1000 we've seen the interrupt.
  regs[E1000_ICR];
}
