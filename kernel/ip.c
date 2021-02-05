#include "arp.h"
#include "ethernet.h"
#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "net.h"
#include "types.h"
#include "udp.h"

static uint
is_ip_packet_valid(struct ip_hdr *hdr)
{
  // TODO: check validity.
  // just return 1 for now.
  return 1;
}

static ushort
in_cksum(const uchar *addr, uint len)
{
  uint nleft = len;
  const ushort *w = (const ushort *)addr;
  uint sum = 0;
  ushort answer = 0;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)  {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(uchar *)(&answer) = *(const uchar *)w;
    sum += answer;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum & 0xffff) + (sum >> 16);
  sum += (sum >> 16);
  /* guaranteed now that the lower 16 bits of sum are correct */

  answer = ~sum; /* truncate to 16 bits */
  return answer;
}

void
ip_tx(struct mbuf *m, uint32 dst_ip, uint8 protocol)
{
  struct ip_hdr *hdr = (struct ip_hdr*) mbuf_prepend(m, sizeof(struct ip_hdr*));
  memset(hdr, 0, sizeof(*hdr));
  hdr->ver = 4; // IPv4 version.
  hdr->hdr = 5;
  hdr->len = toggle_endian16(m->len);
  hdr->ttl = 100;
  hdr->checksum = in_cksum((uchar *) hdr, sizeof(*hdr));
  hdr->src_ip_addr = toggle_endian32(local_ip_addr);
  hdr->dst_ip_addr = toggle_endian32(dst_ip);

  uint8 dst_mac[6];
  if (dst_ip) {
    uint ret = arp_resolve(dst_ip, dst_mac);
    if (ret != 1)
      printf("unresolved ip address.");
  }

  ethernet_tx(m, ETH_TYPE_IPV4, dst_mac);
}

void
ip_rx(struct mbuf *m)
{
  struct ip_hdr *hdr = (struct ip_hdr*)mbuf_pop(m, sizeof(struct ip_hdr*));
  uint16 len = toggle_endian16(hdr->len) - sizeof(*hdr);
  if (!is_ip_packet_valid(hdr)) {
    printf("invalid packet");
    return;
  }
  udp_rx(m, len, hdr);
}
