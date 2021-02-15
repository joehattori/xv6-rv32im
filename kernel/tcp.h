#pragma once

#include "ip.h"
#include "mbuf.h"
#include "queue.h"
#include "socket.h"
#include "types.h"

struct tcp_hdr {
  uint16 src_port;
  uint16 dst_port;
  uint32 seq;
  uint32 ack;
  uint8  off;
  uint8  flg;
  uint16 win;
  uint16 checksum;
  uint16 urg;
} __attribute__((packed));

struct tcp_txq_entry {
  struct tcp_hdr *segment;
  uint16 len;
  //struct timeval timestamp;
  struct tcp_txq_entry *next;
};

struct tcp_txq_head {
  struct tcp_txq_entry *head;
  struct tcp_txq_entry *tail;
};

struct tcp_cb {
  uint8 used;
  uint8 state;
  uint16 port;
  struct {
    uint32 addr;
    uint16 port;
  } peer;
  struct {
    uint32 nxt;
    uint32 una;
    uint16 up;
    uint32 wl1;
    uint32 wl2;
    uint16 wnd;
  } snd;
  uint32 iss;
  struct {
    uint32 nxt;
    uint16 up;
    uint16 wnd;
  } rcv;
  uint32 irs;
  struct tcp_txq_head txq;
  uint8 buf[8192];
  struct tcp_cb *parent;
  struct queue_head backlog;
};

int  tcp_open(uint32, uint16);
int  tcp_close(int);
int  tcp_connect(struct mbuf*, int, uint32, uint32);
int  tcp_send(struct mbuf*, int, uint);
void tcp_rx(struct mbuf*, uint16, struct ip_hdr*);
int  tcp_read(struct socket *s, uint32, uint32);
void tcp_init(void);
