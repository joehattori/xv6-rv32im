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

void socket_append_mbuf(struct socket*, struct mbuf*);
void socket_init(void);
int  socket_alloc(struct file**, uint32, uint16, uint8);
int  socket_close(struct socket*);
int  socket_read(struct socket*, uint32, uint);
int  socket_write(struct socket*, uint32, uint32);
void socket_recv_udp(struct mbuf*, uint32, uint16, uint16);
struct socket *search_socket(uint32, uint16, uint16);
