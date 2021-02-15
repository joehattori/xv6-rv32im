#include "defs.h"
#include "ethernet.h"
#include "file.h"
#include "ip.h"
#include "mbuf.h"
#include "spinlock.h"
#include "tcp.h"
#include "types.h"
#include "udp.h"
#include "socket.h"

void
socket_append_mbuf(struct socket *s, struct mbuf *m)
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
socket_alloc(struct file **f, uint32 remote_ip_addr, uint16 remote_port, uint8 type)
{
  // check if there is already a matched socket.
  struct socket *cur = sockets;
  while (cur) {
    if (cur->remote_ip_addr == remote_ip_addr &&
      cur->remote_port == remote_port) {
      release(&lock);
      if (*f)
        fileclose(*f);
      printf("socket already matched\n");
      return -1;
    }
    cur = cur->nxt;
  }

  *f = filealloc();
  struct socket *s = (struct socket*) kalloc();
  s->remote_ip_addr = remote_ip_addr;
  s->remote_port = remote_port;
  initlock(&s->lock, "socket");
  s->mbufs = 0;
  s->type = type;
  (*f)->type = FD_SOCKET;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = s;
  acquire(&lock);
  s->nxt = sockets;
  sockets = s;
  release(&lock);

  if (type == SOCKET_TYPE_TCP) {
    s->tcp_cb_offset = tcp_open();
    struct mbuf *m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct tcp_hdr));
    if (tcp_connect(m, s->tcp_cb_offset, remote_ip_addr, remote_port) < 0)
      printf("socket_alloc(): failed to tcp_connect\n");
  }
  return 0;
}

int
socket_close(struct socket *s)
{
  acquire(&s->lock);
  wakeup(&s->mbufs);
  release(&s->lock);

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
  switch (s->type) {
  case SOCKET_TYPE_UDP:
    return udp_read(s, addr, len);
  case SOCKET_TYPE_TCP:
    return tcp_read(s, addr, len);
  default:
    panic("unhandled socket type.");
  }
}

int
socket_write(struct socket *s, uint32 addr, uint len)
{
  struct proc *p = myproc();
  struct mbuf *m;
  switch (s->type) {
  case SOCKET_TYPE_TCP:
    m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct tcp_hdr));
    mbuf_append(m, len);
    if (copyin(p->pagetable, m->head, addr, len) < 0) {
      mbuf_free(m);
      return -1;
    }
    tcp_send(m, s->tcp_cb_offset, len);
    break;
  case SOCKET_TYPE_UDP:
    m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr));
    mbuf_append(m, len);
    if (copyin(p->pagetable, m->head, addr, len) < 0) {
      mbuf_free(m);
      return -1;
    }
    udp_tx(m, s->remote_ip_addr, s->local_port, s->remote_port);
    break;
  }
  return len;
}

struct socket *
search_socket(uint32 src_ip_addr, uint16 src_port, uint16 dst_port)
{
  struct socket *s = sockets;
  acquire(&lock);
  // find the corresponding socket.
  while (s) {
    if (s->remote_ip_addr == src_ip_addr && s->local_port == dst_port && s->remote_port == src_port)
      break;
    s = s->nxt;
  }
  release(&lock);
  return s;
}
