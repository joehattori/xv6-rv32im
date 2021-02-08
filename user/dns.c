#include "kernel/types.h"

#include "dns.h"
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
      l = c+1; // skip .
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
static void
dns_rep(uint8 *ibuf, int cc)
{
  struct dns_hdr *hdr = (struct dns_hdr*) ibuf;
  int len;
  char *qname = 0;
  int record = 0;

  if(!hdr->qr) {
    exit(1);
    printf("Not a DNS response for %d\n", toggle_endian16(hdr->id));
  }

  if(hdr->id != toggle_endian16(6828))
    printf("DNS wrong id: %d\n", toggle_endian16(hdr->id));
  
  if(hdr->rcode != 0) {
    printf("DNS rcode error: %x\n", hdr->rcode);
    exit(1);
  }

  len = sizeof(struct dns_hdr);

  for(int i =0; i < toggle_endian16(hdr->req_num); i++) {
    char *qn = (char *) (ibuf+len);
    qname = qn;
    decode_qname(qn);
    len += strlen(qn)+1;
    len += sizeof(struct dns_question);
  }

  for(int i = 0; i < toggle_endian16(hdr->ans_num); i++) {
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
    if(toggle_endian16(d->type) == ARECORD && toggle_endian16(d->len) == 4) {
      record = 1;
      printf("DNS arecord for %s is ", qname ? qname : "" );
      uint8 *ip = (ibuf+len);
      printf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
      if(ip[0] != 128 || ip[1] != 52 || ip[2] != 129 || ip[3] != 126) {
        printf("wrong ip address");
        exit(1);
      }
      len += 4;
    }
  }

  if(len != cc) {
    printf("Processed %d data bytes but received %d\n", len, cc);
    exit(1);
  }
  if(!record) {
    printf("Didn't receive an arecord\n");
    exit(1);
  }
}

void
dns_lookup(char *name)
{
  #define N 1000
  uint8 obuf[N];
  uint8 ibuf[N];
  int fd;
  int len;

  memset(obuf, 0, N);
  memset(ibuf, 0, N);
  
  if((fd = connect(GOOGLE_NAME_SERVER, 10000, 53)) < 0){
    fprintf(2, "ping: connect() failed\n");
    exit(1);
  }

  len = dns_req(obuf, name);
  
  if(write(fd, obuf, len) < 0){
    fprintf(2, "dns: send() failed\n");
    exit(1);
  }
  int cc = read(fd, ibuf, sizeof(ibuf));
  if(cc < 0){
    fprintf(2, "dns: recv() failed\n");
    exit(1);
  }
  dns_rep(ibuf, cc);

  close(fd);
}
