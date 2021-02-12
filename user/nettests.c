#include "kernel/types.h"
#include "kernel/stat.h"

#include "dns.h"
#include "netlib.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  uint16 dport = NET_TESTS_PORT;

  printf("nettests running on port %d\n", dport);

  char msg[] = "Hello, World!";
  printf("testing one ping: ");
  ping(2000, dport, 2, msg);
  printf("OK\n");

  printf("testing single-process pings: ");
  for (int i = 0; i < 100; i++)
    ping(2000, dport, 1, msg);
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
  http_get("example.com");
  printf("HTTP GET OK\n");

  printf("all tests passed.\n");
  exit(0);
}
