#include <stdio.h>

#include "Encoding.hh"
#include "UnitTest.hh"


int main(int argc, char* argv[]) {
  expect_eq(0x2301, bswap16(0x0123));
  expect_eq(0x452301, bswap24(0x012345));
  expect_eq(0x674523, bswap24(0x01234567));
  expect_eq(0x67452301, bswap32(0x01234567));
  expect_eq(0xEFCDAB8967452301, bswap64(0x0123456789ABCDEF));

  expect_eq((uint16_t)-1, bswap16(-1));
  expect_eq((uint32_t)0x00FFFFFF, bswap24(-1));
  expect_eq((int32_t)-1, bswap24s(-1));
  expect_eq((uint32_t)-1, bswap32(-1));
  expect_eq((uint64_t)-1, bswap64(-1));

  expect_eq((uint16_t)-257, bswap16(-2));
  expect_eq((int32_t)-65537, bswap24s(-2));
  expect_eq((uint32_t)-16777217, bswap32(-2));
  expect_eq((uint64_t)-72057594037927937, bswap64(-2));

  expect_eq(2.6f, bswap32f((uint32_t)0x66662640));
  expect_eq(0x66662640UL, bswap32f(2.6f));
  expect_eq(3.1, bswap64f((uint64_t)0xCDCCCCCCCCCC0840));
  expect_eq(0xCDCCCCCCCCCC0840, bswap64f(3.1));

  // TODO: test custom alphabets
  expect_eq("", base64_encode("", 0));
  expect_eq("MQ==", base64_encode("1", 1));
  expect_eq("MTE=", base64_encode("11", 2));
  expect_eq("MTEx", base64_encode("111", 3));
  expect_eq("MTExMg==", base64_encode("1112", 4));
  expect_eq("MTExMjI=", base64_encode("11122", 5));
  expect_eq("MTExMjIy", base64_encode("111222", 6));

  expect_eq("", base64_decode("", 0));
  expect_eq("1", base64_decode("MQ==", 4));
  expect_eq("11", base64_decode("MTE=", 4));
  expect_eq("111", base64_decode("MTEx", 4));
  expect_eq("1112", base64_decode("MTExMg==", 8));
  expect_eq("11122", base64_decode("MTExMjI=", 8));
  expect_eq("111222", base64_decode("MTExMjIy", 8));

  expect_eq("Ab znggre ubj uneq lbh gel...", rot13("No matter how hard you try...", 29));

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
