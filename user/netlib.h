#pragma once

#include "kernel/types.h"

#define ARECORD 1
#define CNAME   5
#define QCLASS  0x1
#define GOOGLE_NAME_SERVER ((8 << 24) | (8 << 16) | (8 << 8) | 8)

struct dns_hdr {
  uint16 id;

  uint8 rd: 1;
  uint8 tc: 1;
  uint8 aa: 1;
  uint8 opcode: 4;
  uint8 qr: 1;

  uint8 rcode: 4;
  uint8 z: 3;
  uint8 ra: 1;

  uint16 req_num;
  uint16 ans_num;
  uint16 rr_num;
  uint16 additional_rr_num;
} __attribute__((packed));

struct dns_question {
  uint16 type;
  uint16 cls;
};

struct dns_data {
  uint16 type;
  uint16 cls;
  uint32 ttl;
  uint16 len;
} __attribute__((packed));

uint32 dns_lookup(char *);

uint http_get(const char *);
void ping(uint16, int, char*);
