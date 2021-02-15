#include "defs.h"
#include "ethernet.h"
#include "file.h"
#include "ip.h"
#include "spinlock.h"
#include "tcp.h"
#include "types.h"
#include "udp.h"
#include "socket.h"

static struct socket *sockets;
static struct spinlock lock;

void
socket_init(void)
{
  initlock(&lock, "socket_lock");
}

int
socket_alloc(struct file **f, uint32 remote_ip_addr, uint16 remote_port, uint8 type)
{
  *f = filealloc();
  struct socket *s = (struct socket*) kalloc();
  s->type = type;
  (*f)->type = FD_SOCKET;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = s;
  acquire(&lock);
  s->nxt = sockets;
  sockets = s;
  release(&lock);

  struct mbuf *m;
  switch (type) {
  case SOCKET_TYPE_TCP:
    s->cb_idx = tcp_open(remote_ip_addr, remote_port);
    m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct tcp_hdr));
    if (tcp_connect(m, s->cb_idx, remote_ip_addr, remote_port) < 0)
      printf("socket_alloc(): failed to tcp_connect\n");
    break;
  case SOCKET_TYPE_UDP:
    s->cb_idx = udp_open(remote_ip_addr, remote_port);
    m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct tcp_hdr));
    if (udp_connect(m, s->cb_idx, remote_ip_addr, remote_port) < 0)
      printf("socket_alloc(): failed to udp_connect\n");
    break;
  default:
    printf("socket_alloc(): unhandled socket type\n");
    return -1;
  }
  return 0;
}

int
socket_close(struct socket *s)
{
  switch (s->type) {
  case SOCKET_TYPE_TCP:
    if (tcp_close(s->cb_idx) < 0) {
      printf("tcp close failed\n");
      return -1;
    }
    break;
  case SOCKET_TYPE_UDP:
    if (udp_close(s->cb_idx) < 0) {
      printf("udp close failed\n");
      return -1;
    }
    break;
  default:
    printf("socket_close(): unhandled socket type.\n");
    return -1;
  }

  acquire(&lock);
  struct socket* shead = sockets;
  if (!shead->nxt)
    sockets = 0;
  while (shead->nxt) {
    if (shead == 0)
      break;
    if (shead->nxt->type == s->type && shead->nxt->cb_idx == s->cb_idx) {
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
    tcp_send(m, s->cb_idx, len);
    break;
  case SOCKET_TYPE_UDP:
    m = mbuf_alloc(sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr));
    mbuf_append(m, len);
    if (copyin(p->pagetable, m->head, addr, len) < 0) {
      mbuf_free(m);
      return -1;
    }
    udp_send(m, s->cb_idx, len);
    break;
  }
  return len;
}
