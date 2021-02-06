#include "defs.h"
#include "ethernet.h"
#include "file.h"
#include "ip.h"
#include "mbuf.h"
#include "spinlock.h"
#include "types.h"
#include "udp.h"

struct socket {
  struct socket *nxt;
  uint32 remote_ip_addr;
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

int
socket_alloc(struct file **f, uint32 remote_ip_addr, uint16 local_port, uint16 remote_port)
{
  struct socket *s = (struct socket*) kalloc();
  s->remote_ip_addr = remote_ip_addr;
  s->local_port = local_port;
  s->remote_port = remote_port;
  s->mbufs = 0;
  (*f)->type = FD_SOCKET;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = s;
  // TODO: check if there is already a matched socket.
  acquire(&s->lock);
  s->nxt = sockets;
  sockets = s;
  release(&s->lock);
  return 0;
}

int
socket_close(struct socket *s)
{
  // TODO: wakeup?
  acquire(&lock);
  struct socket* shead = sockets;
  if (!shead->nxt)
    sockets = 0;
  while (shead->nxt) {
    if (shead == 0)
      break;
    if (shead->nxt->remote_ip_addr == s->remote_ip_addr &&
      shead->nxt->local_port == s->local_port &&
      shead->nxt->remote_port == s->remote_port) {
      shead->nxt = s->nxt;
      break;
    }
    shead = shead->nxt;
  }
  release(&lock);
  kfree(s);
  return 0;
}

int
socket_read(struct socket *s, uint32 addr, uint len)
{
  struct proc *p = myproc();
  acquire(&s->lock);
  struct mbuf *m = s->mbufs;
  if (len > m->len)
    len = m->len;

  if (copyout(p->pagetable, addr, m->head, len) < 0) {
    release(&s->lock);
    mbuf_free(m);
    return -1;
  }

  release(&s->lock);
  mbuf_free(m);
  return 0;
}

int
socket_write(struct socket *s, uint32 addr, uint len)
{
  struct proc *p = myproc();
  struct mbuf *m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr));
  mbuf_append(m, len);
  if (copyin(p->pagetable, m->head, addr, len) < 0) {
    mbuf_free(m);
    return -1;
  }
  udp_tx(m, s->remote_ip_addr, s->local_port, s->remote_port);
  return 0;
}

void
socket_recv_udp(struct mbuf *m, uint32 src_ip_addr, uint16 src_port, uint16 dst_port)
{
  struct socket *s = sockets;
  acquire(&lock);
  // find the corresponding socket.
  while (s) {
    if (s->remote_ip_addr == src_ip_addr && s->local_port == src_port && s->remote_port == dst_port)
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
