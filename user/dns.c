#include "kernel/types.h"

#include "netlib.h"
#include "user.h"

// Encode a DNS name
static void
encode_qname(char *qn, char *host)
{
  char *l = host;
  
  for(char *c = host; c < host+strlen(host)+1; c++) {
    if(*c == '.') {
      *qn++ = (char) (c-l);
      for(char *d = l; d < c; d++) {
        *qn++ = *d;
      }
      l = c+1; // skip '.'
    }
  }
  *qn = '\0';
}

// Decode a DNS name
static void
decode_qname(char *qn)
{
  while(*qn != '\0') {
    int l = *qn;
    if(l == 0)
      break;
    for(int i = 0; i < l; i++) {
      *qn = *(qn+1);
      qn++;
    }
    *qn++ = '.';
  }
}

// Make a DNS request
static int
dns_req(uint8 *obuf, char *name)
{
  int len = 0;
  
  struct dns_hdr *hdr = (struct dns_hdr*) obuf;
  hdr->id = toggle_endian16(6828);
  hdr->rd = 1;
  hdr->req_num = toggle_endian16(1);
  
  len += sizeof(struct dns_hdr);
  
  // qname part of question
  char *qname = (char *) (obuf + sizeof(struct dns_hdr));
  encode_qname(qname, name);
  len += strlen(qname) + 1;

  // constants part of question
  struct dns_question *h = (struct dns_question *) (qname+strlen(qname)+1);
  h->type = toggle_endian16(0x1);
  h->cls = toggle_endian16(0x1);

  len += sizeof(struct dns_question);
  return len;
}

// Process DNS response
// TODO: handle multiple requests.
static uint32
dns_rep(uint8 *ibuf, const int cc)
{
  struct dns_hdr *hdr = (struct dns_hdr*) ibuf;
  int record = 0;

  if (!hdr->qr) {
    exit(1);
    printf("Not a DNS response for %d\n", toggle_endian16(hdr->id));
  }

  if (hdr->id != toggle_endian16(6828))
    printf("DNS wrong id: %d\n", toggle_endian16(hdr->id));
  
  if (hdr->rcode != 0) {
    printf("DNS rcode error: %x\n", hdr->rcode);
    exit(1);
  }

  int len = sizeof(struct dns_hdr);

  for (int i = 0; i < toggle_endian16(hdr->req_num); i++) {
    char *qn = (char *) (ibuf+len);
    decode_qname(qn);
    len += strlen(qn)+1;
    len += sizeof(struct dns_question);
  }

  uint32 ret = 0;
  for (int i = 0; i < toggle_endian16(hdr->ans_num); i++) {
    char *qn = (char *) (ibuf+len);
    
    if((int) qn[0] > 63) {  // compression?
      qn = (char *)(ibuf+qn[1]);
      len += 2;
    } else {
      decode_qname(qn);
      len += strlen(qn)+1;
    }
      
    struct dns_data *d = (struct dns_data *) (ibuf+len);
    len += sizeof(struct dns_data);
    uint16 type = toggle_endian16(d->type);
    switch (type) {
    case ARECORD:
      if (toggle_endian16(d->len) == 4) {
        record = 1;
        for (int j = 0 ; j < 4; j++)
          ret = (ret << 8) + *(ibuf + len + j);
        len += 4;
      }
      break;
    case CNAME:
      len += toggle_endian16(d->len);
      break;
    default:
      printf("DNS: Unhandled type\n");
    }
  }

  if (len != cc) {
    printf("Processed %d data bytes but received %d\n", len, cc);
    exit(1);
  }
  if (!record) {
    printf("Didn't receive an arecord\n");
    exit(1);
  }
  return ret;
}

uint32
dns_lookup(char *name)
{
  #define N 1000
  uint8 obuf[N], ibuf[N];

  memset(obuf, 0, N);
  memset(ibuf, 0, N);
  
  int fd;
  if((fd = connect(GOOGLE_NAME_SERVER, 53, 1)) < 0){
    fprintf(2, "ping: connect() failed\n");
    exit(1);
  }

  strcpy(name + strlen(name), ".");
  int len = dns_req(obuf, name);
  strcpy(name + strlen(name) - 1, "\0");
  
  if(write(fd, obuf, len) < 0){
    fprintf(2, "dns: send() failed\n");
    exit(1);
  }
  int cc;
  if((cc = read(fd, ibuf, sizeof(ibuf))) < 0){
    fprintf(2, "dns: recv() failed\n");
    exit(1);
  }
  close(fd);
  return dns_rep(ibuf, (const int) cc);
}
