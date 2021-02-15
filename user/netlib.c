#include "kernel/types.h"

#include "netlib.h"
#include "user.h"

static void
split_uri(const char *url, char *host_buf, char *path_buf)
{
  char *p = strchr(url, '/');
  if (!p) {
    strcpy(path_buf, "/");
    strcpy(host_buf, url);
    return;
  }
  strcpy(path_buf, p);
  memmove(host_buf, url, strlen(url) - strlen(p));
}

static char *
build_request_line(char *buf, char *host, char *path)
{
  strcpy(buf + strlen(buf), "GET ");
  strcpy(buf + strlen(buf), path);
  strcpy(buf + strlen(buf), " HTTP/1.1\r\nHost: ");
  strcpy(buf + strlen(buf), host);
  strcpy(buf + strlen(buf), "\r\n\r\n");
  return buf;
}

uint
http_get(const char *url)
{
  char *host = malloc(80);
  char *path = malloc(80);
  split_uri(url, host, path);
  uint32 ip_addr = dns_lookup(host);
  uint32 remote_port = 80;

  int fd;
  if ((fd = connect(ip_addr, remote_port, 0)) < 0) {
    fprintf(2, "http: connect() failed\n");
    exit(1);
  }

  // send
  uint sent_bytes_count = 0;
  char *req_line = malloc(1000);
  build_request_line(req_line, host, path);
  free(host);
  free(path);
  uint total = strlen(req_line);
  while (sent_bytes_count < total) {
    uint bytes = write(fd, req_line + sent_bytes_count, total - sent_bytes_count);
    if (bytes < 0) {
      fprintf(2, "http: write() failed\n");
      exit(1);
    }
    sent_bytes_count += bytes;
  }
  free(req_line);

  // response
  #define RESP_SIZE 4096
  char *resp = malloc(RESP_SIZE * sizeof(char));
  uint received_bytes = 0;
  while (1) {
    memset(resp, 0, RESP_SIZE * sizeof(char));
    uint bytes = read(fd, resp, RESP_SIZE);
    if (bytes < 0) {
      fprintf(2, "http: read() failed\n");
      exit(1);
    }
    if (bytes == 0 && received_bytes > 0)
      break;
    for (int i = 0; i < bytes; i++)
      printf("%c", resp[i]);
    received_bytes += bytes;
  }
  printf("\n");

  free(resp);

  close(fd);
  return 0;
}

void
ping(uint16 dport, int attempts, char *msg)
{
  // 10.0.2.2, which qemu remaps to the external host,
  // i.e. the machine you're running qemu on.
  uint32 dst = (10 << 24) | (0 << 16) | (2 << 8) | (2 << 0);

  // you can send a UDP packet to any Internet address
  // by using a different dst.

  char obuf[1000];
  memmove(obuf, msg, strlen(msg));

  int fd = connect(dst, dport, 1);
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
