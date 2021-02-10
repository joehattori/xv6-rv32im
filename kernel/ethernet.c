#include "arp.h"
#include "defs.h"
#include "ethernet.h"
#include "ip.h"
#include "mbuf.h"
#include "types.h"

const uint8 ETHERNET_ADDR_ANY[6] = {0, 0, 0, 0, 0, 0};
const uint8 ETHERNET_ADDR_BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

void
ethernet_tx(struct mbuf *m, uint16 type, const uint8 dst_mac[6])
{
  struct ethernet_hdr *hdr = (struct ethernet_hdr*) mbuf_prepend(m, sizeof(struct ethernet_hdr));
  memmove(hdr->dst_mac, dst_mac, 6);
  memmove(hdr->src_mac, LOCAL_MAC_ADDR, 6);
  hdr->type = toggle_endian16(type);
  if (e1000_send(m) < 0) {
    printf("failed to send packet.\n");
    mbuf_free(m);
  }
}

void
ethernet_rx(struct mbuf *m)
{
  struct ethernet_hdr *hdr = (struct ethernet_hdr*) mbuf_pop(m, sizeof(struct ethernet_hdr));
  if (!hdr) {
    mbuf_free(m);
    panic("ethernet_rx\n");
    return;
  }
  switch (toggle_endian16(hdr->type)) {
    case ETH_TYPE_IPV4:
      ip_rx(m);
      return;
    case ETH_TYPE_ARP:
      arp_rx(m);
      return;
    default:
      printf("unhandled ethernet type.\n");
      mbuf_free(m);
  }
}
