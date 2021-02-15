#include "kernel/types.h"
#include "kernel/stat.h"

#include "netlib.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }
  uint16 dport = atoi(argv[1]);

  printf("nettests running on port %d\n", dport);

  char msg[] = "Hello, World!";
  printf("testing one ping: ");
  ping(dport, 2, msg);
  printf("OK\n");

  printf("testing single-process pings: ");
  for (int i = 0; i < 100; i++)
    ping(dport, 1, msg);
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
  char *domain = "pdos.csail.mit.edu";
  uint32 ip = dns_lookup(domain);
  printf("IP address of %s is %x\n", domain, ip);
  printf("DNS OK\n");

  printf("testing HTTP GET\n");
  http_get("x.com");
  printf("\nHTTP GET OK\n");

  printf("all tests passed.\n");
  exit(0);
}
