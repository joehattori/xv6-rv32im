#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "types.h"
#include "udp.h"
#include "socket.h"
#include "proc.h"

static uint
is_udp_packet_valid(struct udp_hdr *hdr)
{
  // TODO: check validity.
  // just return 1 for now.
  return 1;
}

void
udp_tx(struct mbuf *m, uint32 dst_ip, uint16 src_port, uint16 dst_port)
{
  struct udp_hdr *hdr = (struct udp_hdr*) mbuf_prepend(m, sizeof(struct udp_hdr));
  hdr->src_port = toggle_endian16(src_port);
  hdr->dst_port = toggle_endian16(dst_port);
  hdr->len = toggle_endian16(m->len);
  hdr->checksum = 0; // provide no checksum.
  ip_tx(m, dst_ip, IP_PROTO_UDP);
}

void
udp_rx(struct mbuf *m, uint16 len, struct ip_hdr *iphdr)
{
  if (len < sizeof(struct udp_hdr))
    return;
  struct udp_hdr *udphdr = (struct udp_hdr*) mbuf_pop(m, sizeof(struct udp_hdr));
  if (!is_udp_packet_valid(udphdr))
    return;

  uint32 src_ip_addr = toggle_endian32(iphdr->src_ip_addr);
  uint16 src_port = toggle_endian16(udphdr->src_port);
  uint16 dst_port = toggle_endian16(udphdr->dst_port);

  struct socket *s = search_socket(src_ip_addr, src_port, dst_port);
  // when no socket found
  if (s == 0) {
    printf("socket not found\n");
    mbuf_free(m);
    return;
  }

  acquire(&s->lock);
  socket_append_mbuf(s, m);
  release(&s->lock);
  wakeup(&s->mbufs);
}

int
udp_read(struct socket *s, uint32 addr, uint len)
{
  struct proc *p = myproc();
  acquire(&s->lock);

  // sleep the process while socket's mbufs is empty.
  if (!s->mbufs) {
    while (!s->mbufs)
      sleep(&s->mbufs, &s->lock);
  }
  struct mbuf *m = s->mbufs;
  s->mbufs = m->nxt;
  if (len > m->len)
    len = m->len;

  if (copyout(p->pagetable, addr, m->head, len) < 0) {
    release(&s->lock);
    mbuf_free(m);
    return -1;
  }

  release(&s->lock);
  mbuf_free(m);
  return len;
}
