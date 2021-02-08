#pragma once

#include "types.h"
#include "mbuf.h"

#define ETH_TYPE_IPV4 0x800
#define ETH_TYPE_ARP  0x806

extern const uint8 ETHERNET_ADDR_ANY[6];
extern const uint8 ETHERNET_ADDR_BROADCAST[6];

struct ethernet_hdr {
  uint8 dst_mac[6];
  uint8 src_mac[6];
  uint16 type;
} __attribute__((packed));

void ethernet_tx(struct mbuf*, uint16, const uint8[6]);
void ethernet_rx(struct mbuf*);
