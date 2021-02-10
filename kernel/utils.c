#include "types.h"
#include "defs.h"

uint16
toggle_endian16(uint16 v)
{
  return ((0xff & v) << 8) | ((0xff00 & v) >> 8);
}

uint32
toggle_endian32(uint32 v)
{
  return (((v & 0xff) << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8) | ((v & 0xff000000) >> 24));
}

int
time(int *t)
{
  acquire(&tickslock);
  int tmp = ticks / 100;
  release(&tickslock);
  if (t)
    *t = tmp;
  return tmp;
}

static int
do_rand(unsigned long *ctx)
{
/*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * without overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
  long hi, lo, x;

  /* Transform to [1, 0x7ffffffe] range. */
  x = (*ctx % 0x7ffffffe) + 1;
  hi = x / 127773;
  lo = x % 127773;
  x = 16807 * lo - 2836 * hi;
  if (x < 0)
      x += 0x7fffffff;
  /* Transform to [0, 0x7ffffffd] range. */
  x--;
  *ctx = x;
  return (x);
}

unsigned long rand_next = 1;

int
rand(void)
{
  return do_rand(&rand_next);
}
