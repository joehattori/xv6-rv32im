#include "types.h"
#include "defs.h"
#include "socket.h"
#include "spinlock.h"
#include "ethernet.h"
#include "tcp.h"
#include "ip.h"
#include "mbuf.h"
#include "proc.h"
#include "queue.h"

#define TCP_CB_TABLE_SIZE 16
#define TCP_SOURCE_PORT_MIN 49152
#define TCP_SOURCE_PORT_MAX 65535

#define TCP_CB_STATE_CLOSED      0
#define TCP_CB_STATE_LISTEN      1
#define TCP_CB_STATE_SYN_SENT    2
#define TCP_CB_STATE_SYN_RCVD    3
#define TCP_CB_STATE_ESTABLISHED 4
#define TCP_CB_STATE_FIN_WAIT1   5
#define TCP_CB_STATE_FIN_WAIT2   6
#define TCP_CB_STATE_CLOSING     7
#define TCP_CB_STATE_TIME_WAIT   8
#define TCP_CB_STATE_CLOSE_WAIT  9
#define TCP_CB_STATE_LAST_ACK   10

#define TCP_FLG_FIN 0x01
#define TCP_FLG_SYN 0x02
#define TCP_FLG_RST 0x04
#define TCP_FLG_PSH 0x08
#define TCP_FLG_ACK 0x10
#define TCP_FLG_URG 0x20

#define TCP_FLG_IS(x, y) ((x & 0x3f) == (y))
#define TCP_FLG_ISSET(x, y) ((x & 0x3f) & (y))

#define TCP_CB_LISTENER_SIZE 128

#define TCP_CB_STATE_RX_ISREADY(x) (x->state == TCP_CB_STATE_ESTABLISHED || x->state == TCP_CB_STATE_FIN_WAIT1 || x->state == TCP_CB_STATE_FIN_WAIT2)
#define TCP_CB_STATE_TX_ISREADY(x) (x->state == TCP_CB_STATE_ESTABLISHED || x->state == TCP_CB_STATE_CLOSE_WAIT)

#define TCP_SOCKET_ISINVALID(x) (x < 0 || x >= TCP_CB_TABLE_SIZE)

static struct spinlock lock;
struct tcp_cb cb_table[TCP_CB_TABLE_SIZE];

static int
tcp_txq_add(struct tcp_cb *cb, struct tcp_hdr *hdr, uint len)
{
  struct tcp_txq_entry *txq;

  txq = (struct tcp_txq_entry*) kalloc();
  if (!txq)
    return -1;
  txq->segment = (struct tcp_hdr*) kalloc();
  if (!txq->segment) {
    kfree((char*)txq);
    return -1;
  }
  memmove(txq->segment, hdr, len);
  txq->len = len;
  txq->next = 0;

  // set txq to next of tail entry
  if (cb->txq.head == 0) {
    cb->txq.head = txq;
  } else {
    cb->txq.tail->next = txq;
  }
  // update tail entry
  cb->txq.tail = txq;

  return 0;
}

static int
tcp_cb_clear(struct tcp_cb *cb)
{
  struct tcp_txq_entry *txq;
  struct tcp_cb *backlog;

  while (cb->txq.head) {
    txq = cb->txq.head;
    cb->txq.head = txq->next;
    kfree((char*) txq->segment);
    kfree((char*) txq);
  }
  while (1) {
    struct queue_entry *entry = queue_pop(&cb->backlog);
    if (!entry)
      break;
    backlog = entry->data;
    kfree((char*) entry);
    tcp_cb_clear(backlog);
  }
  memset(cb, 0, sizeof(*cb));
  return 0;
}

static int
tcp_tx(struct mbuf *m, struct tcp_cb *cb, uint32 seq, uint32 ack, uint8 flg, uint len)
{
  struct tcp_hdr *hdr = (struct tcp_hdr*) mbuf_prepend(m, sizeof(struct tcp_hdr));
  memset(hdr, 0, sizeof(*hdr));
  hdr->src_port = cb->port;
  hdr->dst_port = cb->peer.port;
  hdr->seq = toggle_endian32(seq);
  hdr->ack = toggle_endian32(ack);
  hdr->off = (sizeof(struct tcp_hdr) >> 2) << 4;
  hdr->flg = flg;
  hdr->win = toggle_endian16(cb->rcv.wnd);
  hdr->checksum = 0;
  hdr->urg = 0;

  uint32 self = toggle_endian32(LOCAL_IP_ADDR);
  uint32 peer = cb->peer.addr;
  uint32 pseudo = 0;
  pseudo += (self >> 16) & 0xffff;
  pseudo += self & 0xffff;
  pseudo += (peer >> 16) & 0xffff;
  pseudo += peer & 0xffff;
  pseudo += toggle_endian16((uint16) IP_PROTO_TCP);
  pseudo += toggle_endian16(sizeof(struct tcp_hdr) + len);
  hdr->checksum = checksum((uint8*) hdr, sizeof(struct tcp_hdr) + len, pseudo);

  ip_tx(m, toggle_endian32(peer), IP_PROTO_TCP);
  tcp_txq_add(cb, hdr, sizeof(struct tcp_hdr) + len);
  return len;
}

static void
tcp_incoming_event(struct mbuf *m, struct tcp_cb *cb, struct tcp_hdr *hdr, uint len)
{
  uint32 seq, ack;

  uint hlen = ((hdr->off >> 4) << 2);
  uint plen = len - hlen;
  switch (cb->state) {
    case TCP_CB_STATE_CLOSED:
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_RST))
        return;
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_ACK)) {
        seq = toggle_endian32(hdr->ack);
        ack = 0;
      } else {
        seq = 0;
        ack = toggle_endian32(hdr->seq);
        if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_SYN))
          ack++;
        if (plen)
          ack += plen;
        if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_FIN))
          ack++;
      }
      tcp_tx(m, cb, seq, ack, TCP_FLG_RST, 0);
      return;
    case TCP_CB_STATE_LISTEN:
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_RST))
        return;
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_ACK)) {
        seq = toggle_endian32(hdr->ack);
        ack = 0;
        tcp_tx(m, cb, seq, ack, TCP_FLG_RST, 0);
        return;
      }
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_SYN)) {
        cb->rcv.nxt = toggle_endian32(hdr->seq) + 1;
        cb->irs = toggle_endian32(hdr->seq);
        cb->iss = (uint32) rand();
        seq = cb->iss;
        ack = cb->rcv.nxt;
        tcp_tx(m, cb, seq, ack, TCP_FLG_SYN | TCP_FLG_ACK, 0);
        cb->snd.nxt = cb->iss + 1;
        cb->snd.una = cb->iss;
        cb->state = TCP_CB_STATE_SYN_RCVD;
      }
      return;
    case TCP_CB_STATE_SYN_SENT:
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_ACK)) {
        if (toggle_endian32(hdr->ack) <= cb->iss || toggle_endian32(hdr->ack) > cb->snd.nxt) {
          if (!TCP_FLG_ISSET(hdr->flg, TCP_FLG_RST)) {
            seq = toggle_endian32(hdr->ack);
            ack = 0;
            tcp_tx(m, cb, seq, ack, TCP_FLG_RST, 0);
          }
          return;
        }
      }
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_RST)) {
        if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_ACK)) {
          // TCB close
        }
        return;
      }
      if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_SYN)) {
        cb->rcv.nxt = toggle_endian32(hdr->seq) + 1;
        cb->irs = toggle_endian32(hdr->seq);
        if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_ACK)) {
          cb->snd.una = toggle_endian32(hdr->ack);
          // delete TX queue
          if (cb->snd.una > cb->iss) {
            cb->state = TCP_CB_STATE_ESTABLISHED;
            seq = cb->snd.nxt;
            ack = cb->rcv.nxt;
            tcp_tx(m, cb, seq, ack, TCP_FLG_ACK, 0);
            wakeup(cb);
          }
          return;
        }
        seq = cb->iss;
        ack = cb->rcv.nxt;
        tcp_tx(m, cb, seq, ack, TCP_FLG_ACK, 0);
      }
      return;
    default:
      break;
  }
  if (toggle_endian32(hdr->seq) != cb->rcv.nxt) {
    // TODO
    return;
  }
  if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_RST | TCP_FLG_SYN)) {
    // TODO
    return;
  }
  if (!TCP_FLG_ISSET(hdr->flg, TCP_FLG_ACK)) {
    // TODO
    return;
  }
  switch (cb->state) {
    case TCP_CB_STATE_SYN_RCVD:
      if (cb->snd.una <= toggle_endian32(hdr->ack) && toggle_endian32(hdr->ack) <= cb->snd.nxt) {
        cb->state = TCP_CB_STATE_ESTABLISHED;
        queue_push(&cb->parent->backlog, cb, sizeof(*cb));
        wakeup(cb->parent);
      } else {
        tcp_tx(m, cb, toggle_endian32(hdr->ack), 0, TCP_FLG_RST, 0);
        break;
      }
    case TCP_CB_STATE_ESTABLISHED:
    case TCP_CB_STATE_FIN_WAIT1:
    case TCP_CB_STATE_FIN_WAIT2:
    case TCP_CB_STATE_CLOSE_WAIT:
    case TCP_CB_STATE_CLOSING:
      if (cb->snd.una < toggle_endian32(hdr->ack) && toggle_endian32(hdr->ack) <= cb->snd.nxt) {
        cb->snd.una = toggle_endian32(hdr->ack);
      } else if (toggle_endian32(hdr->ack) > cb->snd.nxt) {
        tcp_tx(m, cb, cb->snd.nxt, cb->rcv.nxt, TCP_FLG_ACK, 0);
        return;
      }
      // send window update
      if (cb->state == TCP_CB_STATE_FIN_WAIT1) {
        if (toggle_endian32(hdr->ack) == cb->snd.nxt)
          cb->state = TCP_CB_STATE_FIN_WAIT2;
      } else if (cb->state == TCP_CB_STATE_CLOSING) {
        if (toggle_endian32(hdr->ack) == cb->snd.nxt) {
          cb->state = TCP_CB_STATE_TIME_WAIT;
          wakeup(cb);
        }
        return;
      }
      break;
    case TCP_CB_STATE_LAST_ACK:
      wakeup(cb);
      tcp_cb_clear(cb);
      return;
  }
  if (plen) {
    switch (cb->state) {
      case TCP_CB_STATE_ESTABLISHED:
      case TCP_CB_STATE_FIN_WAIT1:
      case TCP_CB_STATE_FIN_WAIT2:
        memmove(cb->window + (sizeof(cb->window) - cb->rcv.wnd), (uint8 *)hdr + hlen, plen);
        cb->rcv.nxt = toggle_endian32(hdr->seq) + plen;
        cb->rcv.wnd -= plen;
        seq = cb->snd.nxt;
        ack = cb->rcv.nxt;
        tcp_tx(m, cb, seq, ack, TCP_FLG_ACK, 0);
        wakeup(cb);
        break;
      default:
        break;
    }
  }
  if (TCP_FLG_ISSET(hdr->flg, TCP_FLG_FIN)) {
    cb->rcv.nxt++;
    tcp_tx(m, cb, cb->snd.nxt, cb->rcv.nxt, TCP_FLG_ACK, 0);
    switch (cb->state) {
      case TCP_CB_STATE_SYN_RCVD:
      case TCP_CB_STATE_ESTABLISHED:
        cb->state = TCP_CB_STATE_CLOSE_WAIT;
        wakeup(cb);
        break;
      case TCP_CB_STATE_FIN_WAIT1:
        cb->state = TCP_CB_STATE_FIN_WAIT2;
        break;
      case TCP_CB_STATE_FIN_WAIT2:
        cb->state = TCP_CB_STATE_TIME_WAIT;
        wakeup(cb);
        break;
      default:
        break;
    }
    return;
  }
  return;
}

void
tcp_rx(struct mbuf *m, uint16 len, struct ip_hdr *iphdr)
{
  struct tcp_hdr *hdr = (struct tcp_hdr*) mbuf_pop(m, sizeof(struct tcp_hdr));
  struct tcp_cb *cb, *fcb = 0, *lcb = 0;

  if (len < sizeof(struct tcp_hdr))
    return;
  uint32 src_ip_addr = iphdr->src_ip_addr;
  uint16 src_port = hdr->src_port;
  uint16 dst_port = hdr->dst_port;
  acquire(&lock);
  for (cb = cb_table; cb < cb_table + TCP_CB_TABLE_SIZE; cb++) {
    if (!cb->used) {
      if (!fcb)
        fcb = cb;
    } else if (cb->port == dst_port) {
      if (cb->peer.addr == src_ip_addr && cb->peer.port == src_port)
        break;
      if (cb->state == TCP_CB_STATE_LISTEN && !lcb)
        lcb = cb;
    }
  }
  if (cb == cb_table + TCP_CB_TABLE_SIZE) {
    if (!lcb || !fcb || !TCP_FLG_IS(hdr->flg, TCP_FLG_SYN)) {
      // send RST
      release(&lock);
      return;
    }
    cb = fcb;
    cb->used = 1;
    cb->state = lcb->state;
    cb->port = lcb->port;
    cb->peer.addr = src_ip_addr;
    cb->peer.port = src_port;
    cb->rcv.wnd = sizeof(cb->window);
    cb->parent = lcb;
  }
  uint8 data_offset = hdr->off >> 4;
  // 1 word = 4 bytes
  uint16 diff = data_offset * 4 - sizeof(*hdr);
  if (data_offset < 5 || 15 < data_offset)
    printf("invalid data offset in tcp header.\n");
  mbuf_trim(m, diff);
  uint default_size = sizeof(struct ethernet_hdr) + sizeof(struct ip_hdr) + sizeof(struct tcp_hdr);
  tcp_incoming_event(mbuf_alloc(default_size), cb, hdr, len - diff);
  release(&lock);
  socket_recv_tcp(m, toggle_endian32(src_ip_addr), toggle_endian16(src_port), toggle_endian16(dst_port));
}

int
tcp_open()
{
  acquire(&lock);
  for (struct tcp_cb *cb = cb_table; cb < cb_table + TCP_CB_TABLE_SIZE; cb++) {
    if (cb->used)
      continue;
    cb->used = 1;
    release(&lock);
    return ((uint32) cb - (uint32) cb_table) / sizeof(*cb);
  }
  release(&lock);
  return -1;
}

int
tcp_close(struct mbuf *m, int soc)
{
  if (TCP_SOCKET_ISINVALID(soc))
    return -1;
  acquire(&lock);
  struct tcp_cb *cb = &cb_table[soc];
  if (!cb->used) {
    release(&lock);
    return -1;
  }
  switch (cb->state) {
    case TCP_CB_STATE_SYN_RCVD:
    case TCP_CB_STATE_ESTABLISHED:
      tcp_tx(m, cb, cb->snd.nxt, cb->rcv.nxt, TCP_FLG_FIN | TCP_FLG_ACK, 0);
      cb->state = TCP_CB_STATE_FIN_WAIT1;
      cb->snd.nxt++;
      sleep(cb, &lock);
      break;
    case TCP_CB_STATE_CLOSE_WAIT:
      tcp_tx(m, cb, cb->snd.nxt, cb->rcv.nxt, TCP_FLG_FIN | TCP_FLG_ACK, 0);
      cb->state = TCP_CB_STATE_LAST_ACK;
      cb->snd.nxt++;
      sleep(cb, &lock);
      break;
    default:
      break;
  }
  tcp_cb_clear(cb);
  release(&lock);
  return 0;
}

int
tcp_connect(struct mbuf *m, int soc, uint32 dst_ip, uint32 dst_port, uint32 local_port)
{
  if (TCP_SOCKET_ISINVALID(soc))
    return -1;
  acquire(&lock);
  struct tcp_cb *cb = &cb_table[soc];
  if (!cb->used || cb->state != TCP_CB_STATE_CLOSED) {
    release(&lock);
    return -1;
  }
  if (!cb->port) {
    struct tcp_cb *tmp;
    for (tmp = cb_table; tmp < cb_table + TCP_CB_TABLE_SIZE; tmp++) {
      if (tmp->used && tmp->port == toggle_endian16((uint16) local_port)) {
        break;
      }
    }
    if (tmp == cb_table + TCP_CB_TABLE_SIZE) {
      cb->port = toggle_endian16((uint16) local_port);
    }
    if (!cb->port) {
      release(&lock);
      return -1;
    }
  }
  cb->peer.addr = toggle_endian32(dst_ip);
  cb->peer.port = toggle_endian16((uint16) dst_port);
  cb->rcv.wnd = sizeof(cb->window);
  cb->iss = (uint32) rand();
  tcp_tx(m, cb, cb->iss, 0, TCP_FLG_SYN, 0);
  cb->snd.nxt = cb->iss + 1;
  cb->state = TCP_CB_STATE_SYN_SENT;
  while (cb->state == TCP_CB_STATE_SYN_SENT)
    sleep(&cb_table[soc], &lock);
  release(&lock);
  return 0;
}

int
tcp_send(struct mbuf *m, int soc, uint len)
{
  if (TCP_SOCKET_ISINVALID(soc))
    return -1;
  acquire(&lock);
  struct tcp_cb *cb = &cb_table[soc];
  if (!cb->used) {
    release(&lock);
    return -1;
  }
  if (!TCP_CB_STATE_TX_ISREADY(cb)) {
    release(&lock);
    return -1;
  }
  tcp_tx(m, cb, cb->snd.nxt, cb->rcv.nxt, TCP_FLG_ACK | TCP_FLG_PSH, len);
  cb->snd.nxt += len;
  release(&lock);
  return 0;
}

void
tcp_init()
{
  initlock(&lock, "lock");
}
