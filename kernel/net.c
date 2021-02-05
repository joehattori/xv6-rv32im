#include "types.h"
#include "defs.h"
#include "mbuf.h"
#include "arp.h"
#include "ethernet.h"
#include "ip.h"

const uint32 local_ip_addr = ((10 << 24) | (0 << 16) | (2 << 8) | (15 << 0));
const uint8 local_mac_addr[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};

uint16
toggle_endian16(uint16 v)
{
  return ((0xff & v) << 8) | ((0xff00 & v) >> 8);
}

uint32
toggle_endian32(uint32 v)
{
  return (((v & 0xff) << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | ((v & 0xff000000) >> 24));
}

void
net_rx(struct mbuf *m)
{
  struct ethernet_hdr *hdr = (struct ethernet_hdr*) mbuf_pop(m, sizeof(struct ethernet_hdr*));
  if (!hdr) {
    mbuf_free(m);
    return;
  }
  uint16 type = toggle_endian16(hdr->type);
  switch (type) {
    case ETH_TYPE_IPV4:
      ip_rx(m);
      return;
    case ETH_TYPE_ARP:
      arp_rx(m);
      return;
    default:
      mbuf_free(m);
  }
}
