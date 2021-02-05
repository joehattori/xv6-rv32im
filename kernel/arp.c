#include "arp.h"
#include "defs.h"
#include "ethernet.h"
#include "mbuf.h"
#include "net.h"

#define ARP_OR_REQUEST 1
#define ARP_OR_REPLY   2

#define ETH_HTYPE  1
#define IPV4_PTYPE 0x800
#define ETH_HLEN   6
#define IPV4_PLEN  4

static uint32 local_ip_addr = (10 << 24) | (0 << 16) | (2 << 8) | (15 << 0);
static uint8 local_mac_addr[] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};

static int
arp_tx(uint16 op, uint8 dmac[6], uint32 dst_ip)
{
  struct mbuf *m = mbuf_alloc(MBUF_DEFAULT_HEADROOM);
  if (!m)
    return -1;
  struct arp *arp_hdr = (struct arp*) mbuf_append(m, sizeof(struct arp*));
  arp_hdr->htype = ETH_HTYPE;
  arp_hdr->ptype = IPV4_PTYPE;
  arp_hdr->hlen = ETH_HLEN;
  arp_hdr->plen = IPV4_PLEN;
  arp_hdr->oper = toggle_endian16(op);
  memmove(arp_hdr->sha, local_mac_addr, 6);
  arp_hdr->spa = toggle_endian32(local_ip_addr);
  memmove(arp_hdr->tha, dmac, 6);
  arp_hdr->tpa = toggle_endian32(dst_ip);
  ethernet_tx(m, ETH_TYPE_ARP);
  return 0;
}

void
arp_rx(struct mbuf *m)
{
  struct arp *arp_hdr = (struct arp*) mbuf_pop(m, sizeof(struct arp*));

  // TODO: validate packet

  uint32 target_ip = toggle_endian32(arp_hdr->tpa);
  uint8 sender_address[6];
  memmove(sender_address, arp_hdr->sha, 6);
  arp_tx(ARP_OR_REPLY, sender_address, target_ip);
}
