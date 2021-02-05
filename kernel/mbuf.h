#pragma once

#include "types.h"

#define MBUF_SIZE 2048

struct mbuf {
  struct mbuf *nxt;
  char *head;
  uint len;
  char buf[MBUF_SIZE];
};

struct mbuf* mbuf_alloc(uint);
void         mbuf_free(struct mbuf*);
char*        mbuf_append(struct mbuf*, uint);
char*        mbuf_pop(struct mbuf*, uint);
char*        mbuf_trim(struct mbuf*, uint);
