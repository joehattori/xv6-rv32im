#pragma once

#include "types.h"
#include "mbuf.h"

extern const uint32 LOCAL_IP_ADDR;
extern const uint8 LOCAL_MAC_ADDR[6];
extern const uint32 GATEWAY_IP_ADDR;
extern const uint8 GATEWAY_MAC_ADDR[6];

#define IP_ADDR_BROADCAST 0xffffffff
#define IP_PROTO_UDP 17
#define IP_PROTO_TCP 6

#define USABLE_PORT_MIN 49152
#define USABLE_PORT_MAX 65535

struct ip_hdr {
  uint8  hdr: 4; // header field should come before version field because of the endian difference.
  uint8  ver: 4;
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

uint16 checksum(const uint8*, uint, uint);
void ip_tx(struct mbuf*, uint32, uint8);
void ip_rx(struct mbuf*);
