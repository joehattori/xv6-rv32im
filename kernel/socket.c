#include "types.h"
#include "mbuf.h"
#include "defs.h"
#include "spinlock.h"

struct socket {
  struct socket *nxt;
  uint32 ip_addr;
  uint16 local_port;
  uint16 remote_port;
  struct spinlock lock;
  struct mbuf *mbufs;
};

static void
append_mbuf(struct socket *s, struct mbuf *m)
{
  m->nxt = 0;
  if (s->mbufs == 0) {
    s->mbufs = m;
    return;
  }
  struct mbuf *tail = mbuf_get_tail(s->mbufs);
  tail->nxt = m;
}

static struct socket *sockets;
static struct spinlock lock;

void
socket_init(void)
{
  initlock(&lock, "socket");
}

void
socket_recv_udp(struct mbuf *m, uint32 src_ip_addr, uint16 src_port, uint16 dst_port)
{
  struct socket *s = sockets;
  acquire(&lock);
  // find the corresponding socket.
  while (s) {
    if (s->ip_addr == src_ip_addr && s->local_port == src_port && s->remote_port == dst_port)
      break;
    s = s->nxt;
  }
  release(&lock);

  // when no socket found
  if (s == 0) {
    mbuf_free(m);
    return;
  }

  acquire(&s->lock);
  append_mbuf(s, m);
  release(&s->lock);
  wakeup(&s->mbufs);
}
