#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "types.h"
#include "udp.h"
#include "socket.h"
#include "spinlock.h"
#include "proc.h"

#define UDP_CB_TABLE_SIZE 16

#define UDP_SOCKET_ISINVALID(x) (x < 0 || x >= UDP_CB_TABLE_SIZE)

static struct spinlock lock;
static struct udp_cb cb_table[UDP_CB_TABLE_SIZE];

static uint
is_udp_packet_valid(struct udp_hdr *hdr)
{
  // TODO: check validity.
  // just return 1 for now.
  return 1;
}

static void
append_mbuf(struct udp_cb *cb, struct mbuf *m)
{
  m->nxt = 0;
  if (cb->mbufs == 0) {
    cb->mbufs = m;
    return;
  }
  struct mbuf *tail = mbuf_get_tail(cb->mbufs);
  tail->nxt = m;
}

void
udp_tx(struct mbuf *m, struct udp_cb *cb, uint len)
{
  struct udp_hdr *hdr = (struct udp_hdr*) mbuf_prepend(m, sizeof(struct udp_hdr));
  hdr->src_port = cb->port;
  hdr->dst_port = cb->peer.port;
  hdr->len = toggle_endian16(m->len);
  hdr->checksum = 0; // provide no checksum.
  ip_tx(m, toggle_endian32(cb->peer.addr), IP_PROTO_UDP);
}

void
udp_rx(struct mbuf *m, uint16 len, struct ip_hdr *iphdr)
{
  if (len < sizeof(struct udp_hdr))
    return;
  struct udp_hdr *udphdr = (struct udp_hdr*) mbuf_pop(m, sizeof(struct udp_hdr));
  if (!is_udp_packet_valid(udphdr))
    return;

  uint32 src_ip_addr = iphdr->src_ip_addr;
  uint16 src_port = udphdr->src_port;
  uint16 dst_port = udphdr->dst_port;

  struct udp_cb *cb;
  for (cb = cb_table; cb < cb_table + UDP_CB_TABLE_SIZE; cb++)
    if (cb->used && cb->peer.addr == src_ip_addr && cb->peer.port == src_port && cb->port == dst_port)
      break;

  if (cb == cb_table + UDP_CB_TABLE_SIZE) {
    printf("socket not found in udp_rx\n");
    mbuf_free(m);
    return;
  }

  acquire(&cb->lock);
  append_mbuf(cb, m);
  release(&cb->lock);
  wakeup(&cb->mbufs);
}

int
udp_open(uint32 ip, uint16 port)
{
  acquire(&lock);
  for (struct udp_cb *cb = cb_table; cb < cb_table + UDP_CB_TABLE_SIZE; cb++) {
    if (cb->used) {
      if (cb->peer.addr == ip && cb->peer.port == port) {
        printf("udp_open(): reusing same address and port\n");
        return -1;
      }
      continue;
    }
    cb->used = 1;
    release(&lock);
    return ((uint32) cb - (uint32) cb_table) / sizeof(*cb);
  }
  release(&lock);
  return -1;
}

int
udp_close(int soc)
{
  if (UDP_SOCKET_ISINVALID(soc))
    return -1;

  acquire(&lock);
  struct udp_cb *cb = &cb_table[soc];

  if (!cb->used) {
    release(&lock);
    return -1;
  }
  memset(cb, 0, sizeof(*cb));
  release(&lock);
  return 0;
}

int
udp_connect(struct mbuf *m, int soc, uint32 dst_ip, uint32 dst_port)
{
  if (UDP_SOCKET_ISINVALID(soc))
    return -1;

  acquire(&lock);
  struct udp_cb *cb = &cb_table[soc];
  if (!cb->used) {
    release(&lock);
    return -1;
  }

  cb->peer.addr = toggle_endian32(dst_ip);
  cb->peer.port = toggle_endian16((uint16) dst_port);

  if (!cb->port) {
    for (uint port = USABLE_PORT_MIN; port <= USABLE_PORT_MAX; port++) {
      struct udp_cb *_cb;
      for (_cb = cb_table; _cb < cb_table + UDP_CB_TABLE_SIZE; _cb++)
        if (_cb->used && _cb->port == toggle_endian16(port))
          break;
      if (_cb == cb_table + UDP_CB_TABLE_SIZE) {
        cb->port = toggle_endian16(port);
        break;
      }
    }
    if (!cb->port) {
      release(&lock);
      return -1;
    }
  }
  initlock(&cb->lock, "udp_cb_lock");
  release(&lock);
  return 0;
}

int
udp_send(struct mbuf *m, int soc, uint len)
{
  if (UDP_SOCKET_ISINVALID(soc))
    return -1;

  acquire(&lock);
  struct udp_cb *cb = &cb_table[soc];
  if (!cb->used) {
    release(&lock);
    return -1;
  }
  udp_tx(m, cb, len);
  release(&lock);
  return 0;
}

int
udp_read(struct socket *s, uint32 addr, uint len)
{
  struct proc *p = myproc();
  struct udp_cb *cb = &cb_table[s->cb_idx];
  acquire(&cb->lock);

  // sleep the process while socket's mbufs is empty.
  if (!cb->mbufs)
    while (!cb->mbufs)
      sleep(&cb->mbufs, &cb->lock);

  struct mbuf *m = cb->mbufs;
  cb->mbufs = m->nxt;
  if (len > m->len)
    len = m->len;

  if (copyout(p->pagetable, addr, m->head, len) < 0) {
    release(&cb->lock);
    mbuf_free(m);
    return -1;
  }

  release(&cb->lock);
  mbuf_free(m);
  return len;
}

void
udp_init()
{
  initlock(&lock, "udplock");
}
