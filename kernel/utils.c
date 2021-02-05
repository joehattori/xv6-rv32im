#include "types.h"

uint16
toggle_endian(uint16 v)
{
  return (0xff & v) | (0xff00 & v);
}
