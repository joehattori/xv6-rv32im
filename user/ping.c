#include "kernel/types.h"
#include "kernel/stat.h"

#include "dns.h"
#include "netlib.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  uint16 dport = atoi(argv[1]);

  if (argc < 3) {
    fprintf(2, "Usage: ping <port> <whatever you want>\n");
    exit(1);
  }
  ping(2000, dport, 1, argv[2]);
  exit(0);
}
