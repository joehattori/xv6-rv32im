#include "defs.h"
#include "ethernet.h"
#include "net.h"
#include "mbuf.h"
#include "types.h"

const uint8 ETHERNET_ADDR_ANY[6] = {"\x00\x00\x00\x00\x00\x00"};
const uint8 ETHERNET_ADDR_BROADCAST[6] = {"\xff\xff\xff\xff\xff\xff"};

void
ethernet_tx(struct mbuf *m, uint16 type, const uint8 dst_mac[6])
{
  struct ethernet_hdr *hdr = (struct ethernet_hdr*) mbuf_append(m, sizeof(struct ethernet_hdr*));
  memmove(hdr->dst_mac, dst_mac, 6);
  memmove(hdr->src_mac, local_mac_addr, 6);
  hdr->type = type;
  if (e1000_send(m))
    mbuf_free(m);
}
