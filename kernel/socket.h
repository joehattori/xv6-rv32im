#pragma once

#include "types.h"
#include "spinlock.h"

#define SOCKET_TYPE_TCP 0
#define SOCKET_TYPE_UDP 1

struct socket {
  struct socket *nxt;
  uint32 remote_ip_addr;
  uint16 local_port;
  uint16 remote_port;
  struct spinlock lock;
  struct mbuf *mbufs;
  uint8  type;
  uint8  tcp_cb_offset;
};

struct socketaddr_in {
  uint16 sin_port;
  uint32 sin_addr;
};

struct socketaddr {
  char sa_data[14];
};
