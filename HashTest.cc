#include <unistd.h>

#include "Hash.hh"
#include "UnitTest.hh"

using namespace std;


int main(int argc, char* argv[]) {
  {
    printf("-- fnv1a64\n");
    expect_eq(0xCBF29CE484222325, fnv1a64(NULL, 0)); // technically undefined, but should work
    expect_eq(0xCBF29CE484222325, fnv1a64("", 0));
    expect_eq(0xE6CAC1F92EB65713, fnv1a64("omg hax", 7));
    expect_eq(0x594B81FB565E8D30, fnv1a64("lollercoaster", 13));
  }

  {
    printf("-- md5\n");
    expect_eq(string("\xD4\x1D\x8C\xD9\x8F\x00\xB2\x04\xE9\x80\x09\x98\xEC\xF8\x42\x7E", 16),
        md5(NULL, 0)); // technically undefined, but should work
    expect_eq(string("\xD4\x1D\x8C\xD9\x8F\x00\xB2\x04\xE9\x80\x09\x98\xEC\xF8\x42\x7E", 16),
        md5("", 0));
    expect_eq(string("\xFA\xC7\xE1\x8E\xD6\x59\x9B\x37\x7C\x60\xF2\xCA\x94\xCC\xB4\x5B", 16),
        md5("omg hax", 7));
    expect_eq(string("\x9E\x10\x7D\x9D\x37\x2B\xB6\x82\x6B\xD8\x1D\x35\x42\xA4\x19\xD6", 16),
        md5("The quick brown fox jumps over the lazy dog", 43));
  }

  {
    printf("-- sha1\n");
    expect_eq(string("\xDA\x39\xA3\xEE\x5E\x6B\x4B\x0D\x32\x55\xBF\xEF\x95\x60\x18\x90\xAF\xD8\x07\x09", 20),
        sha1(NULL, 0)); // technically undefined, but should work
    expect_eq(string("\xDA\x39\xA3\xEE\x5E\x6B\x4B\x0D\x32\x55\xBF\xEF\x95\x60\x18\x90\xAF\xD8\x07\x09", 20),
        sha1("", 0));
    expect_eq(string("\x6A\x30\xD0\x34\x3E\xD1\x31\x36\x96\xD2\x0B\xCC\x25\xFA\x7E\x2A\xD5\xA9\x77\x7F", 20),
        sha1("omg hax", 7));
    expect_eq(string("\x2F\xD4\xE1\xC6\x7A\x2D\x28\xFC\xED\x84\x9E\xE1\xBB\x76\xE7\x39\x1B\x93\xEB\x12", 20),
        sha1("The quick brown fox jumps over the lazy dog", 43));
  }

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
