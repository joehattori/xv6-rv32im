#pragma once

#include "types.h"
#include "mbuf.h"

struct ip_hdr {
  uint8  ver: 4;
  uint8  hdr: 4;
  uint8  tos;
  uint16 len;
  uint16 id;
  uint16 fragment;
  uint8  ttl;
  uint8  protocol;
  uint16 checksum;
  uint32 src_ip_addr;
  uint32 dst_ip_addr;
} __attribute__((packed));

void ip_rx(struct mbuf*);
