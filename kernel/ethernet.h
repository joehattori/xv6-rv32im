#include "types.h"

struct ethernet_hdr {
  uint64 dst_mac: 48;
  uint64 src_mac: 48;
  uint16 type;
} __attribute__((packed));
