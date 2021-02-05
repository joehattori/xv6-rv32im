#include "types.h"
#include "mbuf.h"
#include "defs.h"

struct socket {
  struct socket *nxt;
};

static struct socket *sockets;

void
sock_recv_udp(struct mbuf *m, uint32 src_ip_addr, uint16 src_port, uint16 dst_port)
{
}
