#include "types.h"
#include "defs.h"
#include "ip.h"
#include "mbuf.h"
#include "net.h"
#include "udp.h"

static uint
is_ip_packet_valid(struct ip_hdr *hdr)
{
  // TODO: check validity.
  // just return 1 for now.
  return 1;
}

void
ip_rx(struct mbuf *m)
{
  struct ip_hdr *hdr = (struct ip_hdr*)mbuf_pop(m, sizeof(struct ip_hdr*));
  uint16 len = toggle_endian(hdr->len) - sizeof(*hdr);
  if (!is_ip_packet_valid(hdr)) {
    printf("invalid packet");
    return;
  }
  udp_rx(m, len, hdr);
}
