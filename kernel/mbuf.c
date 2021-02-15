#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "mbuf.h"

struct mbuf *
mbuf_alloc(uint headroom)
{
  if (headroom > MBUF_SIZE)
    return 0;
  struct mbuf *m = kalloc();
  if (m == 0)
    return 0;
  m->nxt = 0;
  m->head = (char*) m->buf + headroom;
  m->len = 0;
  memset(m->buf, 0, sizeof(m->buf));
  return m;
}

void
mbuf_free(struct mbuf *m)
{
  kfree(m);
}

char *
mbuf_append(struct mbuf *m, uint len)
{
  char *ret = m->head + m->len;
  m->len += len;
  if (m->len > MBUF_SIZE)
    panic("mbuf_append");
  return ret;
}

char *
mbuf_trim(struct mbuf *m, uint len)
{
  if (len > m->len)
    return 0;
  m->len -= len;
  return m->head + m->len;
}

char *
mbuf_prepend(struct mbuf *m, uint len)
{
  m->head -= len;
  if (m->head < m->buf) {
    printf("\nhead is preceding %d bytes.\n", m->buf - m->head);
    panic("mbuf_prepend");
  }
  m->len += len;
  return m->head;
}

char *
mbuf_pop(struct mbuf *m, uint len)
{
  char *ret = m->head;
  m->len -= len;
  if (m->len < 0)
    return 0;
  m->head += len;
  return ret;
}

struct mbuf *
mbuf_get_tail(struct mbuf *m)
{
  struct mbuf *cur = m;
  while (1) {
    if (cur->nxt == 0)
      return cur;
    cur = cur->nxt;
  }
}
