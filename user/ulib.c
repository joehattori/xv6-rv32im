#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char *
strcpy(char *s, const char *t)
{
  char *os = s;
  while((*s++ = *t++) != 0);
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
  p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;
  for(n = 0; s[n]; n++);
  return n;
}

void *
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst;
  for(int i = 0; i < n; i++)
  cdst[i] = c;
  return dst;
}

char *
strchr(const char *s, char c)
{
  for(; *s; s++)
  if(*s == c)
    return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i = 0; i + 1 < max;){
  cc = read(0, &c, 1);
  if(cc < 1)
    break;
  buf[i++] = c;
  if(c == '\n' || c == '\r')
    break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
  return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
  n = n*10 + *s++ - '0';
  return n;
}

char *
itoa(int i, char *b)
{
  char const digit[] = "0123456789";
  char* p = b;
  if(i<0){
    *p++ = '-';
    i *= -1;
  }
  int shifter = i;
  do {
    ++p;
    shifter /= 10;
  } while (shifter);
  *p = '\0';
  do {
    *--p = digit[i%10];
    i /= 10;
  } while (i);
  return b;
}

void *
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  if (src > dst) {
  while(n-- > 0)
    *dst++ = *src++;
  } else {
  dst += n;
  src += n;
  while(n-- > 0)
    *--dst = *--src;
  }
  return vdst;
}

int
memcmp(const void *s1, const void *s2, uint n)
{
  const char *p1 = s1, *p2 = s2;
  while (n-- > 0) {
  if (*p1 != *p2) {
    return *p1 - *p2;
  }
  p1++;
  p2++;
  }
  return 0;
}

void *
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

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

// from FreeBSD.
int
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
  return (do_rand(&rand_next));
}
