#include "kernel/types.h"

#include "dns.h"
#include "http.h"
#include "user.h"

uint
http_get(char *host)
{
  uint32 ip_addr = dns_lookup(host);
  uint32 local_port = 2000;
  uint32 remote_port = 80;

  int fd;
  if ((fd = connect(ip_addr, local_port, remote_port)) < 0) {
    fprintf(2, "http: connect() failed\n");
    exit(1);
  }

  // send
  char *msg = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
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
  char resp[512];
  memset(resp, 0, sizeof(resp));
  total = sizeof(resp) - 1;
  uint received_bytes_count = 0;
  while (received_bytes_count < total) {
    printf("%d %d\n", received_bytes_count, total);
    uint bytes = read(fd, resp + received_bytes_count, total - received_bytes_count);
    if (bytes < 0) {
      fprintf(2, "http: read() failed\n");
      exit(1);
    }
    if (bytes == 0)
      break;
    received_bytes_count += bytes;
  }

  printf("Response: %s\n", resp);

  close(fd);
  return 0;
}
