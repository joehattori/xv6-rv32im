#include "types.h"
#include "defs.h"
#include "mbuf.h"
#include "arp.h"
#include "ethernet.h"
#include "ip.h"

#define ETH_TYPE_IPV4 0x800
#define ETH_TYPE_ARP  0x806

uint16
toggle_endian(uint16 v)
{
  return (0xff & v) | (0xff00 & v);
}

void
net_rx(struct mbuf *m)
{
  struct ethernet_hdr *hdr = (struct ethernet_hdr*) mbuf_pop(m, sizeof(struct ethernet_hdr*));
  if (!hdr) {
    mbuf_free(m);
    return;
  }
  uint16 type = toggle_endian(hdr->type);
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
