#include "types.h"
#include "mbuf.h"

#define ETH_TYPE_IPV4 0x800
#define ETH_TYPE_ARP  0x806

struct ethernet_hdr {
  uint64 dst_mac: 48;
  uint64 src_mac: 48;
  uint16 type;
} __attribute__((packed));

void ethernet_tx(struct mbuf*, uint);
