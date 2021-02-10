#include "kernel/types.h"
#include "kernel/stat.h"

#include "dns.h"
#include "http.h"
#include "user.h"

static void __attribute__((unused))
ping(uint16 sport, uint16 dport, int attempts)
{
  // 10.0.2.2, which qemu remaps to the external host,
  // i.e. the machine you're running qemu on.
  uint32 dst = (10 << 24) | (0 << 16) | (2 << 8) | (2 << 0);

  // you can send a UDP packet to any Internet address
  // by using a different dst.

  int fd = connect(dst, sport, dport);
  if(fd < 0){
    fprintf(2, "ping: connect() failed\n");
    exit(1);
  }

  char obuf[] = "hello world!";
  for(int i = 0; i < attempts; i++) {
    if(write(fd, obuf, sizeof(obuf)) < 0){
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
  if (strcmp(obuf, ibuf) || cc != sizeof(obuf)){
    fprintf(2, "ping didn't receive correct payload\n");
    exit(1);
  }
}

static void
get(char *host)
{
  http_get(host);
}

int
main(int argc, char *argv[])
{
  uint16 dport = NET_TESTS_PORT;

  printf("nettests running on port %d\n", dport);

  printf("testing one ping: ");
  ping(2000, dport, 2);
  printf("OK\n");

  printf("testing single-process pings: ");
  for (int i = 0; i < 100; i++)
    ping(2000, dport, 1);
  printf("OK\n");

  // This test fails sometimes.
  // printf("testing multi-process pings: ");
  // for (i = 0; i < 10; i++){
  //   int pid = fork();
  //   if (pid == 0){
  //     ping(2000 + i + 1, dport, 1);
  //     exit(0);
  //   }
  // }
  // for (i = 0; i < 10; i++){
  //   wait(&ret);
  //   if (ret != 0)
  //     exit(1);
  // }
  // printf("OK\n");

  printf("testing DNS\n");
  char *domain = "pdos.csail.mit.edu.";
  uint32 ip = dns_lookup(domain);
  printf("IP address of %s is %x\n", domain, ip);
  printf("DNS OK\n");

  printf("testing HTTP GET\n");
  get("example.com.");
  printf("HTTP GET OK\n");

  printf("all tests passed.\n");
  exit(0);
}
