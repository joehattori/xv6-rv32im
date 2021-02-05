#include "mbuf.h"

struct arp {
  uint16 htype;
  uint16 ptype;
  uint8  hlen;
  uint8  plen;
  uint16 oper;
  uint8  sha[6];
  uint32 spa;
  uint8  tha[6];
  uint32 tpa;
} __attribute__((packed));

void arp_rx(struct mbuf*);
