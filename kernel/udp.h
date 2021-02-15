#pragma once

#include "mbuf.h"
#include "socket.h"
#include "spinlock.h"
#include "types.h"
#include "ip.h"

struct udp_hdr {
  uint16 src_port;
  uint16 dst_port;
  uint16 len;
  uint16 checksum;
};

struct udp_cb {
  uint8 used;
  uint16 port;
  struct {
    uint32 addr;
    uint16 port;
  } peer;
  struct mbuf *mbufs;
  struct spinlock lock;
};

void udp_rx(struct mbuf*, uint16, struct ip_hdr*);
int  udp_send(struct mbuf*, int, uint);
int  udp_open(uint32, uint16);
int  udp_close(int);
int  udp_connect(struct mbuf*, int, uint32, uint32);
int  udp_read(struct socket *s, uint32 addr, uint32 len);
void udp_init();
