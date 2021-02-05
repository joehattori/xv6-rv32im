#pragma once

#include "types.h"

#define MBUF_SIZE             2048
#define MBUF_DEFAULT_HEADROOM 128

struct mbuf {
  struct mbuf *nxt;
  char *head;
  uint len;
  char buf[MBUF_SIZE];
};

struct mbuf* mbuf_alloc(uint);
void         mbuf_free(struct mbuf*);
char*        mbuf_append(struct mbuf*, uint);
char*        mbuf_trim(struct mbuf*, uint);
char*        mbuf_prepend(struct mbuf*, uint);
char*        mbuf_pop(struct mbuf*, uint);
struct mbuf* mbuf_get_tail(struct mbuf *);
