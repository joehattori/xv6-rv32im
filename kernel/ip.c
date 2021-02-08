#include "arp.h"
#include "ethernet.h"
#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "types.h"
#include "udp.h"

const uint32 LOCAL_IP_ADDR = ((10 << 24) | (0 << 16) | (2 << 8) | (15 << 0));
const uint8 LOCAL_MAC_ADDR[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};

static uint
is_ip_packet_valid(struct ip_hdr *hdr)
{
  // TODO: check validity.
  // just return 1 for now.
  return 1;
}

static uint16
checksum(const uint8 *addr, uint len)
{
  uint16 nleft = len;
  uint32 sum = 0;
  const uint16 *w = (const uint16*) addr;

  while (nleft > 1)  {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft)
    sum += *w & 0xff;

  while (sum >> 16)
    sum = (sum >> 16) + (sum & 0xffff);

  return (uint16) (~sum);
}


void
ip_tx(struct mbuf *m, uint32 dst_ip, uint8 protocol)
{
  struct ip_hdr *hdr = (struct ip_hdr*) mbuf_prepend(m, sizeof(struct ip_hdr));
  memset(hdr, 0, sizeof(*hdr));
  hdr->hdr = 5;
  hdr->ver = 4; // IPv4 version.
  hdr->tos = 0;
  hdr->len = toggle_endian16(m->len);
  hdr->ttl = 100;
  hdr->protocol = protocol;
  hdr->src_ip_addr = toggle_endian32(LOCAL_IP_ADDR);
  hdr->dst_ip_addr = toggle_endian32(dst_ip);

  hdr->checksum = checksum((uint8*) hdr, sizeof(*hdr));

  uint8 dst_mac[6];
  if (dst_ip) {
    arp_resolve(dst_ip, dst_mac);
    // uint ret = arp_resolve(dst_ip, dst_mac);
    // if (ret != 1) {
    //   printf("unresolved ip address %x.\n", dst_ip);
    // }
  } else {
    memmove(dst_mac, ETHERNET_ADDR_BROADCAST, 6);
  }

  ethernet_tx(m, ETH_TYPE_IPV4, dst_mac);
}

void
ip_rx(struct mbuf *m)
{
  struct ip_hdr *hdr = (struct ip_hdr*)mbuf_pop(m, sizeof(struct ip_hdr));
  if (!is_ip_packet_valid(hdr)) {
    printf("invalid packet");
    return;
  }
  uint16 len = toggle_endian16(hdr->len) - sizeof(*hdr);
  udp_rx(m, len, hdr);
}
