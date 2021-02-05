#pragma once

#include "types.h"
#include "ip.h"

struct udp_hdr {
  uint16 src_port;
  uint16 dst_port;
  uint16 len;
  uint16 checksum;
};

void udp_tx(struct mbuf*, uint32, uint16, uint16);
void udp_rx(struct mbuf*, uint16, struct ip_hdr*);
