#include <inttypes.h>
#include <stdio.h>

#include "Encoding.hh"
#include "UnitTest.hh"

int main(int, char**) {
  expect_eq(0x0000, (sign_extend<uint16_t, uint8_t>(0x00)));
  expect_eq(0x0001, (sign_extend<uint16_t, uint8_t>(0x01)));
  expect_eq(0x007F, (sign_extend<uint16_t, uint8_t>(0x7F)));
  expect_eq(0xFF80, (sign_extend<uint16_t, uint8_t>(0x80)));
  expect_eq(0xFFFF, (sign_extend<uint16_t, uint8_t>(0xFF)));
  expect_eq(0x00000000, (sign_extend<uint32_t, uint8_t>(0x00)));
  expect_eq(0x00000001, (sign_extend<uint32_t, uint8_t>(0x01)));
  expect_eq(0x0000007F, (sign_extend<uint32_t, uint8_t>(0x7F)));
  expect_eq(0xFFFFFF80, (sign_extend<uint32_t, uint8_t>(0x80)));
  expect_eq(0xFFFFFFFF, (sign_extend<uint32_t, uint8_t>(0xFF)));
  expect_eq(0x0000000000000000, (sign_extend<uint64_t, uint8_t>(0x00)));
  expect_eq(0x0000000000000001, (sign_extend<uint64_t, uint8_t>(0x01)));
  expect_eq(0x000000000000007F, (sign_extend<uint64_t, uint8_t>(0x7F)));
  expect_eq(0xFFFFFFFFFFFFFF80, (sign_extend<uint64_t, uint8_t>(0x80)));
  expect_eq(0xFFFFFFFFFFFFFFFF, (sign_extend<uint64_t, uint8_t>(0xFF)));
  expect_eq(0x00000000, (sign_extend<uint32_t, uint16_t>(0x0000)));
  expect_eq(0x00000001, (sign_extend<uint32_t, uint16_t>(0x0001)));
  expect_eq(0x00007FFF, (sign_extend<uint32_t, uint16_t>(0x7FFF)));
  expect_eq(0xFFFF8000, (sign_extend<uint32_t, uint16_t>(0x8000)));
  expect_eq(0xFFFFFFFF, (sign_extend<uint32_t, uint16_t>(0xFFFF)));
  expect_eq(0x0000000000000000, (sign_extend<uint64_t, uint16_t>(0x0000)));
  expect_eq(0x0000000000000001, (sign_extend<uint64_t, uint16_t>(0x0001)));
  expect_eq(0x0000000000007FFF, (sign_extend<uint64_t, uint16_t>(0x7FFF)));
  expect_eq(0xFFFFFFFFFFFF8000, (sign_extend<uint64_t, uint16_t>(0x8000)));
  expect_eq(0xFFFFFFFFFFFFFFFF, (sign_extend<uint64_t, uint16_t>(0xFFFF)));
  expect_eq(0x0000000000000000, (sign_extend<uint64_t, uint32_t>(0x00000000)));
  expect_eq(0x0000000000000001, (sign_extend<uint64_t, uint32_t>(0x00000001)));
  expect_eq(0x000000007FFFFFFF, (sign_extend<uint64_t, uint32_t>(0x7FFFFFFF)));
  expect_eq(0xFFFFFFFF80000000, (sign_extend<uint64_t, uint32_t>(0x80000000)));
  expect_eq(0xFFFFFFFFFFFFFFFF, (sign_extend<uint64_t, uint32_t>(0xFFFFFFFF)));

  expect_eq(0, (sign_extend<int16_t, uint8_t>(0x00)));
  expect_eq(1, (sign_extend<int16_t, uint8_t>(0x01)));
  expect_eq(127, (sign_extend<int16_t, uint8_t>(0x7F)));
  expect_eq(-128, (sign_extend<int16_t, uint8_t>(0x80)));
  expect_eq(-1, (sign_extend<int16_t, uint8_t>(0xFF)));
  expect_eq(0, (sign_extend<int32_t, uint8_t>(0x00)));
  expect_eq(1, (sign_extend<int32_t, uint8_t>(0x01)));
  expect_eq(127, (sign_extend<int32_t, uint8_t>(0x7F)));
  expect_eq(-128, (sign_extend<int32_t, uint8_t>(0x80)));
  expect_eq(-1, (sign_extend<int32_t, uint8_t>(0xFF)));
  expect_eq(0, (sign_extend<int64_t, uint8_t>(0x00)));
  expect_eq(1, (sign_extend<int64_t, uint8_t>(0x01)));
  expect_eq(127, (sign_extend<int64_t, uint8_t>(0x7F)));
  expect_eq(-128, (sign_extend<int64_t, uint8_t>(0x80)));
  expect_eq(-1, (sign_extend<int64_t, uint8_t>(0xFF)));
  expect_eq(0, (sign_extend<int32_t, uint16_t>(0x0000)));
  expect_eq(1, (sign_extend<int32_t, uint16_t>(0x0001)));
  expect_eq(32767, (sign_extend<int32_t, uint16_t>(0x7FFF)));
  expect_eq(-32768, (sign_extend<int32_t, uint16_t>(0x8000)));
  expect_eq(-1, (sign_extend<int32_t, uint16_t>(0xFFFF)));
  expect_eq(0, (sign_extend<int64_t, uint16_t>(0x0000)));
  expect_eq(1, (sign_extend<int64_t, uint16_t>(0x0001)));
  expect_eq(32767, (sign_extend<int64_t, uint16_t>(0x7FFF)));
  expect_eq(-32768, (sign_extend<int64_t, uint16_t>(0x8000)));
  expect_eq(-1, (sign_extend<int64_t, uint16_t>(0xFFFF)));
  expect_eq(0, (sign_extend<int64_t, uint32_t>(0x00000000)));
  expect_eq(1, (sign_extend<int64_t, uint32_t>(0x00000001)));
  expect_eq(2147483647, (sign_extend<int64_t, uint32_t>(0x7FFFFFFF)));
  expect_eq(-2147483648, (sign_extend<int64_t, uint32_t>(0x80000000)));
  expect_eq(-1, (sign_extend<int64_t, uint32_t>(0xFFFFFFFF)));

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

  {
    re_uint32_t x;
    x = 3;
    expect_eq(3, x);
    expect_eq(0x03000000, x.load_raw());
    uint32_t y = x;
    expect_eq(3, y);
  }
  {
    re_uint64_t x(0x0102030405060708);
    expect_eq(0x0102030405060708, x);
    expect_eq(0x0807060504030201, x.load_raw());
  }
  {
    re_double x(1.0);
    expect_eq(1.0, x);
    expect_eq(0x000000000000F03F, x.load_raw());
  }

  {
    union {
      uint8_t bytes[4];
      le_uint32_t le32;
      be_uint32_t be32;
    } data;

    data.le32 = 0x01020304;
    expect_eq(data.be32, 0x04030201);
    expect_eq(data.bytes[0], 0x04);
    data.be32 = 0x01020304;
    expect_eq(data.le32, 0x04030201);
    expect_eq(data.bytes[0], 0x01);
  }

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

  expect_eq("The brick quown jox fumps over the dazy log", rot13("Gur oevpx dhbja wbk shzcf bire gur qnml ybt", 43));

  printf("EncodingTest: all tests passed\n");
  return 0;
}
