#include "kernel/types.h"
#include "kernel/stat.h"

#include "netlib.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "Usage: curl <url>\n");
    return 1;
  }
  http_get(argv[1]);
  return 0;
}
