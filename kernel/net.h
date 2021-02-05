#pragma once

#include "mbuf.h"
#include "types.h"

extern const uint32 local_ip_addr;
extern const uint8 local_mac_addr[6];

void   net_rx(struct mbuf*);
uint16 toggle_endian16(uint16);
uint32 toggle_endian32(uint32);
