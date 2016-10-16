#include <assert.h>
#include <stdio.h>

#include "Encoding.hh"


int main(int argc, char* argv[]) {
  assert(bswap16(0x0123) == 0x2301);
  assert(bswap32(0x01234567) == 0x67452301);
  assert(bswap64(0x0123456789ABCDEF) == 0xEFCDAB8967452301);

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
