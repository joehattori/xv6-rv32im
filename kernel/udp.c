#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "types.h"
#include "udp.h"

#define IP_PROTO_UDP 17

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
  struct udp_hdr *udphdr = (struct udp_hdr*) mbuf_pop(m, sizeof(struct udp_hdr));
  if (!is_udp_packet_valid(udphdr))
    return;
  len -= sizeof(*udphdr);
  mbuf_trim(m, m->len - len);

  uint32 src_ip_addr = toggle_endian32(iphdr->src_ip_addr);
  uint16 src_port = toggle_endian16(udphdr->src_port);
  uint16 dst_port = toggle_endian16(udphdr->dst_port);
  socket_recv_udp(m, src_ip_addr, src_port, dst_port);
}
