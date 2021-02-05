#include "defs.h"
#include "ethernet.h"
#include "net.h"
#include "mbuf.h"
#include "types.h"

void
ethernet_tx(struct mbuf *m, uint16 type, const uint8 dst[6])
{
  struct ethernet_hdr *hdr = (struct ethernet_hdr*) mbuf_pop(m, sizeof(struct ethernet_hdr*));
  memmove(hdr->dst_mac, dst, 6);
  memmove(hdr->src_mac, local_mac_addr, 6);
  hdr->type = type;
  if (e1000_send(m))
    mbuf_free(m);
}
