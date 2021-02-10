#include "defs.h"
#include "queue.h"
#include "types.h"

struct queue_entry *
queue_push (struct queue_head *queue, void *data, uint size) {
  struct queue_entry *entry;

  if (!queue || !data) {
    return 0;
  }
  entry = (struct queue_entry *)kalloc();
  if (!entry) {
    return 0;
  }
  entry->data = data;
  entry->size = size;
  entry->next = 0;
  if (queue->tail) {
    queue->tail->next = entry;
  }
  queue->tail = entry;
  if (!queue->next) {
    queue->next = entry;
  }
  queue->num++;
  return entry;
}

struct queue_entry *
queue_pop (struct queue_head *queue) {
  struct queue_entry *entry;

  if (!queue || !queue->next) {
    return 0;
  }
  entry = queue->next;
  queue->next = entry->next;
  if (!queue->next) {
    queue->tail = 0;
  }
  queue->num--;
  return entry;
}
