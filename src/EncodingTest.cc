#include <inttypes.h>
#include <stdio.h>

#include "Encoding.hh"
#include "UnitTest.hh"

int main(int, char**) {
  expect_eq(0x0000, (phosg::sign_extend<uint16_t, uint8_t>(0x00)));
  expect_eq(0x0001, (phosg::sign_extend<uint16_t, uint8_t>(0x01)));
  expect_eq(0x007F, (phosg::sign_extend<uint16_t, uint8_t>(0x7F)));
  expect_eq(0xFF80, (phosg::sign_extend<uint16_t, uint8_t>(0x80)));
  expect_eq(0xFFFF, (phosg::sign_extend<uint16_t, uint8_t>(0xFF)));
  expect_eq(0x00000000, (phosg::sign_extend<uint32_t, uint8_t>(0x00)));
  expect_eq(0x00000001, (phosg::sign_extend<uint32_t, uint8_t>(0x01)));
  expect_eq(0x0000007F, (phosg::sign_extend<uint32_t, uint8_t>(0x7F)));
  expect_eq(0xFFFFFF80, (phosg::sign_extend<uint32_t, uint8_t>(0x80)));
  expect_eq(0xFFFFFFFF, (phosg::sign_extend<uint32_t, uint8_t>(0xFF)));
  expect_eq(0x0000000000000000, (phosg::sign_extend<uint64_t, uint8_t>(0x00)));
  expect_eq(0x0000000000000001, (phosg::sign_extend<uint64_t, uint8_t>(0x01)));
  expect_eq(0x000000000000007F, (phosg::sign_extend<uint64_t, uint8_t>(0x7F)));
  expect_eq(0xFFFFFFFFFFFFFF80, (phosg::sign_extend<uint64_t, uint8_t>(0x80)));
  expect_eq(0xFFFFFFFFFFFFFFFF, (phosg::sign_extend<uint64_t, uint8_t>(0xFF)));
  expect_eq(0x00000000, (phosg::sign_extend<uint32_t, uint16_t>(0x0000)));
  expect_eq(0x00000001, (phosg::sign_extend<uint32_t, uint16_t>(0x0001)));
  expect_eq(0x00007FFF, (phosg::sign_extend<uint32_t, uint16_t>(0x7FFF)));
  expect_eq(0xFFFF8000, (phosg::sign_extend<uint32_t, uint16_t>(0x8000)));
  expect_eq(0xFFFFFFFF, (phosg::sign_extend<uint32_t, uint16_t>(0xFFFF)));
  expect_eq(0x0000000000000000, (phosg::sign_extend<uint64_t, uint16_t>(0x0000)));
  expect_eq(0x0000000000000001, (phosg::sign_extend<uint64_t, uint16_t>(0x0001)));
  expect_eq(0x0000000000007FFF, (phosg::sign_extend<uint64_t, uint16_t>(0x7FFF)));
  expect_eq(0xFFFFFFFFFFFF8000, (phosg::sign_extend<uint64_t, uint16_t>(0x8000)));
  expect_eq(0xFFFFFFFFFFFFFFFF, (phosg::sign_extend<uint64_t, uint16_t>(0xFFFF)));
  expect_eq(0x0000000000000000, (phosg::sign_extend<uint64_t, uint32_t>(0x00000000)));
  expect_eq(0x0000000000000001, (phosg::sign_extend<uint64_t, uint32_t>(0x00000001)));
  expect_eq(0x000000007FFFFFFF, (phosg::sign_extend<uint64_t, uint32_t>(0x7FFFFFFF)));
  expect_eq(0xFFFFFFFF80000000, (phosg::sign_extend<uint64_t, uint32_t>(0x80000000)));
  expect_eq(0xFFFFFFFFFFFFFFFF, (phosg::sign_extend<uint64_t, uint32_t>(0xFFFFFFFF)));

  expect_eq(0, (phosg::sign_extend<int16_t, uint8_t>(0x00)));
  expect_eq(1, (phosg::sign_extend<int16_t, uint8_t>(0x01)));
  expect_eq(127, (phosg::sign_extend<int16_t, uint8_t>(0x7F)));
  expect_eq(-128, (phosg::sign_extend<int16_t, uint8_t>(0x80)));
  expect_eq(-1, (phosg::sign_extend<int16_t, uint8_t>(0xFF)));
  expect_eq(0, (phosg::sign_extend<int32_t, uint8_t>(0x00)));
  expect_eq(1, (phosg::sign_extend<int32_t, uint8_t>(0x01)));
  expect_eq(127, (phosg::sign_extend<int32_t, uint8_t>(0x7F)));
  expect_eq(-128, (phosg::sign_extend<int32_t, uint8_t>(0x80)));
  expect_eq(-1, (phosg::sign_extend<int32_t, uint8_t>(0xFF)));
  expect_eq(0, (phosg::sign_extend<int64_t, uint8_t>(0x00)));
  expect_eq(1, (phosg::sign_extend<int64_t, uint8_t>(0x01)));
  expect_eq(127, (phosg::sign_extend<int64_t, uint8_t>(0x7F)));
  expect_eq(-128, (phosg::sign_extend<int64_t, uint8_t>(0x80)));
  expect_eq(-1, (phosg::sign_extend<int64_t, uint8_t>(0xFF)));
  expect_eq(0, (phosg::sign_extend<int32_t, uint16_t>(0x0000)));
  expect_eq(1, (phosg::sign_extend<int32_t, uint16_t>(0x0001)));
  expect_eq(32767, (phosg::sign_extend<int32_t, uint16_t>(0x7FFF)));
  expect_eq(-32768, (phosg::sign_extend<int32_t, uint16_t>(0x8000)));
  expect_eq(-1, (phosg::sign_extend<int32_t, uint16_t>(0xFFFF)));
  expect_eq(0, (phosg::sign_extend<int64_t, uint16_t>(0x0000)));
  expect_eq(1, (phosg::sign_extend<int64_t, uint16_t>(0x0001)));
  expect_eq(32767, (phosg::sign_extend<int64_t, uint16_t>(0x7FFF)));
  expect_eq(-32768, (phosg::sign_extend<int64_t, uint16_t>(0x8000)));
  expect_eq(-1, (phosg::sign_extend<int64_t, uint16_t>(0xFFFF)));
  expect_eq(0, (phosg::sign_extend<int64_t, uint32_t>(0x00000000)));
  expect_eq(1, (phosg::sign_extend<int64_t, uint32_t>(0x00000001)));
  expect_eq(2147483647, (phosg::sign_extend<int64_t, uint32_t>(0x7FFFFFFF)));
  expect_eq(-2147483648, (phosg::sign_extend<int64_t, uint32_t>(0x80000000)));
  expect_eq(-1, (phosg::sign_extend<int64_t, uint32_t>(0xFFFFFFFF)));

  expect_eq(0x2301, phosg::bswap16(0x0123));
  expect_eq(0x452301, phosg::bswap24(0x012345));
  expect_eq(0x674523, phosg::bswap24(0x01234567));
  expect_eq(0x67452301, phosg::bswap32(0x01234567));
  expect_eq(0xEFCDAB8967452301, phosg::bswap64(0x0123456789ABCDEF));

  expect_eq((uint16_t)-1, phosg::bswap16(-1));
  expect_eq((uint32_t)0x00FFFFFF, phosg::bswap24(-1));
  expect_eq((int32_t)-1, phosg::bswap24s(-1));
  expect_eq((uint32_t)-1, phosg::bswap32(-1));
  expect_eq((uint64_t)-1, phosg::bswap64(-1));

  expect_eq((uint16_t)-257, phosg::bswap16(-2));
  expect_eq((int32_t)-65537, phosg::bswap24s(-2));
  expect_eq((uint32_t)-16777217, phosg::bswap32(-2));
  expect_eq((uint64_t)-72057594037927937, phosg::bswap64(-2));

  expect_eq(2.6f, phosg::bswap32f((uint32_t)0x66662640));
  expect_eq(0x66662640UL, phosg::bswap32f(2.6f));
  expect_eq(3.1, phosg::bswap64f((uint64_t)0xCDCCCCCCCCCC0840));
  expect_eq(0xCDCCCCCCCCCC0840, phosg::bswap64f(3.1));

  {
    phosg::re_uint32_t x;
    x = 3;
    expect_eq(3, x);
    expect_eq(0x03000000, x.load_raw());
    uint32_t y = x;
    expect_eq(3, y);
  }
  {
    phosg::re_uint64_t x(0x0102030405060708);
    expect_eq(0x0102030405060708, x);
    expect_eq(0x0807060504030201, x.load_raw());
  }
  {
    phosg::re_double x(1.0);
    expect_eq(1.0, x);
    expect_eq(0x000000000000F03F, x.load_raw());
  }

  {
    union {
      uint8_t bytes[4];
      phosg::le_uint32_t le32;
      phosg::be_uint32_t be32;
    } data;

    data.le32 = 0x01020304;
    expect_eq(data.be32, 0x04030201);
    expect_eq(data.bytes[0], 0x04);
    expect_eq("04030201", std::format("{:08X}", data.be32));
    data.be32 = 0x01020304;
    expect_eq(data.le32, 0x04030201);
    expect_eq(data.bytes[0], 0x01);
    expect_eq("04030201", std::format("{:08X}", data.le32));
  }

  // TODO: test custom alphabets
  expect_eq("", phosg::base64_encode("", 0));
  expect_eq("MQ==", phosg::base64_encode("1", 1));
  expect_eq("MTE=", phosg::base64_encode("11", 2));
  expect_eq("MTEx", phosg::base64_encode("111", 3));
  expect_eq("MTExMg==", phosg::base64_encode("1112", 4));
  expect_eq("MTExMjI=", phosg::base64_encode("11122", 5));
  expect_eq("MTExMjIy", phosg::base64_encode("111222", 6));

  expect_eq("", phosg::base64_decode("", 0));
  expect_eq("1", phosg::base64_decode("MQ==", 4));
  expect_eq("11", phosg::base64_decode("MTE=", 4));
  expect_eq("111", phosg::base64_decode("MTEx", 4));
  expect_eq("1112", phosg::base64_decode("MTExMg==", 8));
  expect_eq("11122", phosg::base64_decode("MTExMjI=", 8));
  expect_eq("111222", phosg::base64_decode("MTExMjIy", 8));

  expect_eq("The brick quown jox fumps over the dazy log", phosg::rot13("Gur oevpx dhbja wbk shzcf bire gur qnml ybt", 43));

  phosg::fwrite_fmt(stdout, "EncodingTest: all tests passed\n");
  return 0;
}
