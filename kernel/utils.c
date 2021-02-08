#include "types.h"

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
