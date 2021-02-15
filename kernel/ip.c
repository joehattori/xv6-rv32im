#include "arp.h"
#include "ethernet.h"
#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "types.h"
#include "tcp.h"
#include "udp.h"

#define BUILD_IP_ADDR(v1, v2, v3, v4) (((v1) << 24) | ((v2) << 16) | ((v3) << 8) | ((v4) << 0))

const uint32 LOCAL_IP_ADDR = BUILD_IP_ADDR(10, 0, 2, 15);
const uint8 LOCAL_MAC_ADDR[6] = {0x52, 0x54, 0x0, 0x12, 0x34, 0x56};
const uint32 GATEWAY_IP_ADDR = BUILD_IP_ADDR(10, 0, 2, 2);
const uint8 GATEWAY_MAC_ADDR[6] = {0x52, 0x55, 0xa, 0x0, 0x2, 0x2};

static uint
is_ip_packet_valid(struct ip_hdr *hdr)
{
  // TODO: check validity.
  // just return 1 for now.
  return 1;
}

uint16
checksum(const uint8 *addr, uint len, uint init)
{
  uint16 nleft = len;
  uint32 sum = init;
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

  hdr->checksum = checksum((uint8*) hdr, sizeof(*hdr), 0);

  uint8 dst_mac[6];
  memset(dst_mac, 0, sizeof(dst_mac));
  if (dst_ip) {
    uint ret = arp_resolve(dst_ip, dst_mac);
    if (ret != 1)
      memmove(dst_mac, GATEWAY_MAC_ADDR, 6);
  } else {
    memmove(dst_mac, ETHERNET_ADDR_BROADCAST, 6);
  }

  ethernet_tx(m, ETH_TYPE_IPV4, dst_mac);
}

void
ip_rx(struct mbuf *m)
{
  struct ip_hdr *hdr = (struct ip_hdr*) mbuf_pop(m, sizeof(struct ip_hdr));
  if (!is_ip_packet_valid(hdr)) {
    printf("invalid packet");
    return;
  }
  uint16 len = toggle_endian16(hdr->len) - sizeof(struct ip_hdr);

  mbuf_trim(m, m->len - len);

  uint8 protocol = hdr->protocol;
  switch (protocol) {
  case IP_PROTO_TCP:
    tcp_rx(m, len, hdr);
    return;
  case IP_PROTO_UDP:
    udp_rx(m, len, hdr);
    return;
  default:
    printf("ip_rx: unhandled protocol\n");
  }
}
