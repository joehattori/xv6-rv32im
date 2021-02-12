#include "kernel/types.h"

#include "dns.h"
#include "netlib.h"
#include "user.h"

uint
http_get(char host[])
{
  uint32 ip_addr = dns_lookup(host);
  uint32 local_port = 2000;
  uint32 remote_port = 80;

  int fd;
  if ((fd = connect(ip_addr, local_port, remote_port, 0)) < 0) {
    fprintf(2, "http: connect() failed\n");
    exit(1);
  }

  // send
  char msg[] = "GET / HTTP/1.1\r\nHost: ";
  strcpy(msg + strlen(msg), host);
  strcpy(msg + strlen(msg), "\r\n\r\n");
  uint sent_bytes_count = 0;
  uint total = strlen(msg);
  while (sent_bytes_count < total) {
    uint bytes = write(fd, msg + sent_bytes_count, total - sent_bytes_count);
    if (bytes < 0) {
      fprintf(2, "http: write() failed\n");
      exit(1);
    }
    sent_bytes_count += bytes;
  }

  // response
  char resp[1700];
  memset(resp, 0, sizeof(resp));
  total = sizeof(resp) - 1;
  uint received_bytes_count = 0;
  while (received_bytes_count < total) {
    uint bytes = read(fd, resp + received_bytes_count, total - received_bytes_count);
    if (bytes < 0) {
      fprintf(2, "http: read() failed\n");
      exit(1);
    }
    if (bytes == 0 && received_bytes_count > 0)
      break;
    for (int i = 0; i < bytes; i++)
      printf("%c", resp[received_bytes_count + i]);
    received_bytes_count += bytes;
  }

  close(fd);
  return 0;
}

void
ping(uint16 sport, uint16 dport, int attempts, char msg[])
{
  // 10.0.2.2, which qemu remaps to the external host,
  // i.e. the machine you're running qemu on.
  uint32 dst = (10 << 24) | (0 << 16) | (2 << 8) | (2 << 0);

  // you can send a UDP packet to any Internet address
  // by using a different dst.

  char obuf[1000];
  memmove(obuf, msg, strlen(msg));

  int fd = connect(dst, sport, dport, 1);
  if(fd < 0){
    fprintf(2, "ping: connect() failed\n");
    exit(1);
  }

  for(int i = 0; i < attempts; i++) {
    if(write(fd, obuf, strlen(obuf)) < 0){
      fprintf(2, "ping: send() failed\n");
      exit(1);
    }
  }

  char ibuf[128];
  int cc = read(fd, ibuf, sizeof(ibuf));
  if(cc < 0){
    fprintf(2, "ping: recv() failed\n");
    exit(1);
  }

  close(fd);
  if (strcmp(obuf, ibuf) || cc != strlen(obuf)){
    fprintf(2, "ping didn't receive correct payload\n");
    exit(1);
  }
}
