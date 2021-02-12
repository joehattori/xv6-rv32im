#include "kernel/types.h"
#include "kernel/stat.h"

#include "dns.h"
#include "netlib.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  uint16 dport = NET_TESTS_PORT;

  if (argc < 2) {
    fprintf(2, "Usage: ping <whatever you want>\n");
    exit(1);
  }
  ping(2000, dport, 1, argv[1]);
  exit(0);
}
