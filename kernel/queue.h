#pragma once

#include "types.h"

struct queue_entry {
  void *data;
  uint size;
  struct queue_entry *next;
};

struct queue_head {
  struct queue_entry *next;
  struct queue_entry *tail;
  uint num;
};

struct queue_entry *queue_push(struct queue_head*, void*, uint);
struct queue_entry *queue_pop(struct queue_head*);
