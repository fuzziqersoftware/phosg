#define _STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <sys/time.h>

#include "Filesystem.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;

template <typename... ArgTs>
void print_data_test_case(const string& expected_output, ArgTs... args) {

  // macOS doesn't have fmemopen, so we just write to a file because I'm too
  // lazy to use funopen()
  {
    auto f = fopen_unique("StringsTest-data", "w");
    print_data(f.get(), args...);
  }
  string output_data = load_file("StringsTest-data");

  if (expected_output != output_data) {
    fputs("(print_data) Expected:\n", stderr);
    fwrite(expected_output.data(), 1, expected_output.size(), stderr);
    fputs("(print_data) Actual:\n", stderr);
    fwrite(output_data.data(), 1, output_data.size(), stderr);
    expect(false);
  }

  // Also test format_data - it should produce the same result
  output_data = format_data(args...);
  if (expected_output != output_data) {
    fputs("(format_data) Expected:\n", stderr);
    fwrite(expected_output.data(), 1, expected_output.size(), stderr);
    fputs("(format_data) Actual:\n", stderr);
    fwrite(output_data.data(), 1, output_data.size(), stderr);
    expect(false);
  }
}

void print_data_test() {

  // basic
  string first_bytes("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10);
  fprintf(stderr, "-- [print_data] one line\n");
  print_data_test_case("\
00 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes);
  fprintf(stderr, "-- [print_data] multiple lines, last line partial\n");
  print_data_test_case("\
00 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n\
10 | 61 62 63 64 65 66 67 68 69                      | abcdefghi       \n",
      string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x61\x62\x63\x64\x65\x66\x67\x68\x69", 0x19));

  // with offset width flags
  fprintf(stderr, "-- [print_data] with offset width flags\n");
  print_data_test_case("\
00 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0, "", PrintDataFlags::OFFSET_8_BITS | PrintDataFlags::PRINT_ASCII);
  print_data_test_case("\
200 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0x200, "", PrintDataFlags::OFFSET_8_BITS | PrintDataFlags::PRINT_ASCII);
  print_data_test_case("\
0000 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0, "", PrintDataFlags::OFFSET_16_BITS | PrintDataFlags::PRINT_ASCII);
  print_data_test_case("\
00000000 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0, "", PrintDataFlags::OFFSET_32_BITS | PrintDataFlags::PRINT_ASCII);
  print_data_test_case("\
0000000000000000 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0, "", PrintDataFlags::OFFSET_64_BITS | PrintDataFlags::PRINT_ASCII);
  fprintf(stderr, "-- [print_data] automatic offset width\n");
  print_data_test_case("\
F0 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0xF0);
  print_data_test_case("\
0200 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0x200);
  print_data_test_case("\
00055550 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0x55550);
  print_data_test_case("\
00000007F0000000 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      first_bytes, 0x7F0000000);

  // with address
  fprintf(stderr, "-- [print_data] with address\n");
  print_data_test_case("\
3FFF3039AEC14EE0 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
      string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10),
      0x3FFF3039AEC14EE0);
  fprintf(stderr, "-- [print_data] with non-aligned address\n");
  print_data_test_case("\
3FFF3039AEC14EE0 |          00 01 02 03 04 05 06 07 08 09 0A 0B 0C |                 \n\
3FFF3039AEC14EF0 | 0D 0E 0F                                        |                 \n",
      string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10),
      0x3FFF3039AEC14EE3);
  fprintf(stderr, "-- [print_data] short data with non-aligned address\n");
  print_data_test_case("\
3FFF3039AEC14EE0 |          61 63 65                               |    ace          \n",
      "ace", 3, 0x3FFF3039AEC14EE3);
  fprintf(stderr, "-- [print_data] short data with non-aligned address near beginning\n");
  print_data_test_case("\
3FFF3039AEC14EE0 |    61 63 65                                     |  ace            \n",
      "ace", 3, 0x3FFF3039AEC14EE1);
  fprintf(stderr, "-- [print_data] short data with non-aligned address near end\n");
  print_data_test_case("\
3FFF3039AEC14EE0 |                                     61 63 65    |             ace \n",
      "ace", 3, 0x3FFF3039AEC14EEC);

  // without ascii
  fprintf(stderr, "-- [print_data] without ascii\n");
  print_data_test_case("\
3FFF3039AEC14EE0 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n",
      string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10),
      0x3FFF3039AEC14EE0, "", 0);
  print_data_test_case("\
3FFF3039AEC14EE0 |          00 01 02 03 04 05 06 07 08 09 0A 0B 0C\n\
3FFF3039AEC14EF0 | 0D 0E 0F                                       \n",
      string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10),
      0x3FFF3039AEC14EE3, "", 0);
  print_data_test_case("\
3FFF3039AEC14EE0 |          61 63 65                              \n",
      "ace", 0x3FFF3039AEC14EE3, "", 0);

  // floats
  fprintf(stderr, "-- [print_data] with floats\n");
  string float_data("\0\0\0\0\x56\x6F\x6D\xC3\0\0\0\0\xA5\x5B\xC8\x40\0\0\0\0\0\0\0\0\x6E\x37\x9F\x43\x3E\x51\x3F\x40", 0x20);
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                 \n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 | Vom      [ @    \n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |     n7 C>Q?@    \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_ASCII);
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |      -237.43            0       6.2612            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0       318.43       2.9893             \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_FLOAT);
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                  |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 | Vom      [ @     |      -237.43            0       6.2612            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |     n7 C>Q?@     |            0       318.43       2.9893             \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_ASCII | PrintDataFlags::PRINT_FLOAT);

  // reverse-endian floats
#ifdef PHOSG_LITTLE_ENDIAN
  fprintf(stderr, "-- [print_data] with reverse-endian floats\n");
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |   6.5814e+13            0  -1.9063e-16            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0   1.4207e+28      0.20434             \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_FLOAT | PrintDataFlags::REVERSE_ENDIAN_FLOATS);
#else
  fprintf(stderr, "-- [print_data] with reverse-endian floats\n");
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |      -237.43            0       6.2612            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0       318.43       2.9893             \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_FLOAT | PrintDataFlags::REVERSE_ENDIAN_FLOATS);
#endif
  fprintf(stderr, "-- [print_data] with big-endian floats\n");
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |   6.5814e+13            0  -1.9063e-16            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0   1.4207e+28      0.20434             \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_FLOAT | PrintDataFlags::BIG_ENDIAN_FLOATS);
  fprintf(stderr, "-- [print_data] with little-endian floats\n");
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |      -237.43            0       6.2612            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0       318.43       2.9893             \n",
      float_data, 0x0000000107B50FEC, "", PrintDataFlags::PRINT_FLOAT | PrintDataFlags::LITTLE_ENDIAN_FLOATS);

  // float alignment
  fprintf(stderr, "-- [print_data] with floats and non-aligned addresses\n");
  print_data_test_case("\
0000000107B50FE0 |                                              00 |                                                    \n\
0000000107B50FF0 | 00 00 00 40 00                                  |            2                                       \n",
      string("\0\0\0\0\x40\0", 6), 0x0000000107B50FEF, "", PrintDataFlags::PRINT_FLOAT);
  print_data_test_case("\
00 |    00 00 00 00 00 00 00 00                      |                         0                          \n",
      string("\0\0\0\0\0\0\0\0", 8), 1, "", PrintDataFlags::PRINT_FLOAT);

  // doubles
  fprintf(stderr, "-- [print_data] with doubles\n");
  print_data_test_case("\
00 | 3F F0 00 00 00 00 00 00 41 FC 80 AC EA 2C AA 40 |  3.0387e-319       3350.5\n\
10 | 40 00 00 00 00 00 00 00                         |   3.162e-322             \n",
      string("\x3F\xF0\0\0\0\0\0\0\x41\xFC\x80\xAC\xEA\x2C\xAA\x40\x40\0\0\0\0\0\0\0", 0x18),
      0, "", PrintDataFlags::PRINT_DOUBLE | PrintDataFlags::PRINT_DOUBLE | PrintDataFlags::LITTLE_ENDIAN_FLOATS);
  print_data_test_case("\
00 | 3F F0 00 00 00 00 00 00 41 FC 80 AC EA 2C AA 40 |            1   7.6511e+09\n\
10 | 40 00 00 00 00 00 00 00                         |            2             \n",
      string("\x3F\xF0\0\0\0\0\0\0\x41\xFC\x80\xAC\xEA\x2C\xAA\x40\x40\0\0\0\0\0\0\0", 0x18),
      0, "", PrintDataFlags::PRINT_DOUBLE | PrintDataFlags::BIG_ENDIAN_FLOATS);

  // reverse-endian doubles
  fprintf(stderr, "-- [print_data] with reverse-endian doubles\n");
  print_data_test_case("\
00 | 3F F0 00 00 00 00 00 00 41 FC 80 AC EA 2C AA 40 | ?       A    , @ |            1   7.6511e+09\n\
10 | 40 00 00 00 00 00 00 00                         | @                |            2             \n",
      string("\x3F\xF0\0\0\0\0\0\0\x41\xFC\x80\xAC\xEA\x2C\xAA\x40\x40\0\0\0\0\0\0\0", 0x18),
      0, "", PrintDataFlags::PRINT_DOUBLE | PrintDataFlags::PRINT_ASCII | PrintDataFlags::BIG_ENDIAN_FLOATS);

  // double alignment
  fprintf(stderr, "-- [print_data] with doubles and non-aligned addresses\n");
  print_data_test_case("\
00 |    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |                         0\n\
10 | 00                                              |                          \n",
      string("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16), 1, "", PrintDataFlags::PRINT_DOUBLE);

  // iovecs
  fprintf(stderr, "-- [print_data] with iovecs\n");
  string data1("\0\0\0\x40\0\0", 6);
  string data2("\x80\x3F\0\0", 4);
  string data3("\0", 1);
  vector<struct iovec> iovs;
  iovs.emplace_back(iovec{.iov_base = data1.data(), .iov_len = data1.size()});
  iovs.emplace_back(iovec{.iov_base = data2.data(), .iov_len = data2.size()});
  iovs.emplace_back(iovec{.iov_base = nullptr, .iov_len = 0});
  iovs.emplace_back(iovec{.iov_base = data3.data(), .iov_len = data3.size()});
  print_data_test_case("\
00 | 00 00 00 40 00 00 80 3F 00 00 00                |    @   ?         |            2            1                          \n",
      iovs.data(), iovs.size(), 0, nullptr, 0, PrintDataFlags::PRINT_ASCII | PrintDataFlags::PRINT_FLOAT);
  print_data_test_case("\
00 |             00 00 00 40 00 00 80 3F 00 00 00    |        @   ?     |                         2            1             \n",
      iovs.data(), iovs.size(), 4, nullptr, 0, PrintDataFlags::PRINT_ASCII | PrintDataFlags::PRINT_FLOAT);

  // TODO: test diffing
}

void test_bit_reader() {
  fprintf(stderr, "-- BitReader\n");
  BitReader r("\x01\x02\xFF\x80\xC0", 34);
  expect_eq(0x01, r.read(8));
  expect_eq(0x00, r.read(4));
  expect_eq(0x01, r.read(3));
  expect_eq(0x01FF, r.read(10));
  expect_eq(0x00, r.read(7));
  expect_eq(0x03, r.read(2));
  expect(r.eof());
}

void test_string_reader() {
  fprintf(stderr, "-- StringReader\n");

  string data(
      /*00*/ "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F" // 16 bytes (to be read as various int sizes)
             /*10*/ "\x3F\x80\x00\x00" // 1.0f (big-endian)
             /*14*/ "\x00\x00\x80\x3F" // 1.0f (little-endian)
             /*18*/ "\x3F\xF0\x00\x00\x00\x00\x00\x00" // 1.0 (big-endian)
             /*20*/ "\x00\x00\x00\x00\x00\x00\xF0\x3F" // 1.0 (little-endian)
             /*28*/ "\x11this is a pstring"
             /*3A*/ "and this is a cstring\0",
      80);

  fprintf(stderr, "---- construct\n");
  StringReader r(data);
  expect_eq(r.size(), data.size());

  fprintf(stderr, "---- position getters\n");
  expect_eq(r.where(), 0);
  expect_eq(r.remaining(), data.size());
  expect(!r.eof());

  fprintf(stderr, "---- all\n");
  expect_eq(r.all(), data);
  expect_eq(data.data(), r.peek(0));

  {
    fprintf(stderr, "---- read/pread\n");
    expect_eq(r.read(0x100, false), data);
    expect_eq(r.where(), 0);
    expect_eq(r.remaining(), data.size());
    expect(!r.eof());

    expect_eq(r.pread(0, 0x100), data);
    expect_eq(r.where(), 0);
    expect_eq(r.remaining(), data.size());
    expect(!r.eof());

    expect_eq(r.read(0x100), data);
    expect_eq(r.where(), data.size());
    expect_eq(r.remaining(), 0);
    expect(r.eof());

    expect_eq(r.read(0x100), "");
    expect_eq(r.where(), data.size());
    expect_eq(r.remaining(), 0);
    expect(r.eof());

    expect_eq(r.pread(0, 0x100), data);
    expect_eq(r.where(), data.size());
    expect_eq(r.remaining(), 0);
    expect(r.eof());

    r.go(0);
  }

  {
    fprintf(stderr, "---- readx/preadx\n");
    expect_raises<out_of_range>([&]() {
      r.readx(0x100);
    });

    expect_raises<out_of_range>([&]() {
      r.preadx(0, 0x100);
    });

    expect_eq(r.readx(data.size(), false), data);
    expect_eq(r.where(), 0);
    expect_eq(r.remaining(), data.size());
    expect(!r.eof());

    expect_eq(r.readx(data.size()), data);
    expect_eq(r.where(), data.size());
    expect_eq(r.remaining(), 0);
    expect(r.eof());

    expect_raises<out_of_range>([&]() {
      r.readx(1);
    });

    expect_eq(r.preadx(0, data.size()), data);
    expect_eq(r.where(), data.size());
    expect_eq(r.remaining(), 0);
    expect(r.eof());

    r.go(0);
  }

  {
    fprintf(stderr, "---- get/pget\n");
    struct TestStruct {
      be_uint32_t a;
      le_uint32_t b;
    };
    auto x = r.get<TestStruct>();
    auto y = r.pget<TestStruct>(4);
    expect_eq(x.a, 0x00010203);
    expect_eq(x.b, 0x07060504);
    expect_eq(y.a, 0x04050607);
    expect_eq(y.b, 0x0B0A0908);
  }

  struct GetTestCase {
    const char* name;
    function<void(StringReader&)> fn;
    size_t start_offset;
    size_t expected_advance;
  };

  vector<GetTestCase> get_test_cases({
      {"get_u8", [](StringReader& r) { expect_eq(r.get_u8(), 0x00); }, 0, 1},
      {"get_s8", [](StringReader& r) { expect_eq(r.get_s8(), 0x00); }, 0, 1},
      {"get_u16b", [](StringReader& r) { expect_eq(r.get_u16b(), 0x0001); }, 0, 2},
      {"get_u16l", [](StringReader& r) { expect_eq(r.get_u16l(), 0x0100); }, 0, 2},
      {"get_s16b", [](StringReader& r) { expect_eq(r.get_s16b(), 0x0001); }, 0, 2},
      {"get_s16l", [](StringReader& r) { expect_eq(r.get_s16l(), 0x0100); }, 0, 2},
      {"get_u24b", [](StringReader& r) { expect_eq(r.get_u24b(), 0x000102); }, 0, 3},
      {"get_u24l", [](StringReader& r) { expect_eq(r.get_u24l(), 0x020100); }, 0, 3},
      {"get_s24b", [](StringReader& r) { expect_eq(r.get_s24b(), 0x000102); }, 0, 3},
      {"get_s24l", [](StringReader& r) { expect_eq(r.get_s24l(), 0x020100); }, 0, 3},
      {"get_u32b", [](StringReader& r) { expect_eq(r.get_u32b(), 0x00010203); }, 0, 4},
      {"get_u32l", [](StringReader& r) { expect_eq(r.get_u32l(), 0x03020100); }, 0, 4},
      {"get_s32b", [](StringReader& r) { expect_eq(r.get_s32b(), 0x00010203); }, 0, 4},
      {"get_s32l", [](StringReader& r) { expect_eq(r.get_s32l(), 0x03020100); }, 0, 4},
      {"get_u48b", [](StringReader& r) { expect_eq(r.get_u48b(), 0x000102030405); }, 0, 6},
      {"get_u48l", [](StringReader& r) { expect_eq(r.get_u48l(), 0x050403020100); }, 0, 6},
      {"get_s48b", [](StringReader& r) { expect_eq(r.get_s48b(), 0x000102030405); }, 0, 6},
      {"get_s48l", [](StringReader& r) { expect_eq(r.get_s48l(), 0x050403020100); }, 0, 6},
      {"get_u64b", [](StringReader& r) { expect_eq(r.get_u64b(), 0x0001020304050607); }, 0, 8},
      {"get_u64l", [](StringReader& r) { expect_eq(r.get_u64l(), 0x0706050403020100); }, 0, 8},
      {"get_s64b", [](StringReader& r) { expect_eq(r.get_s64b(), 0x0001020304050607); }, 0, 8},
      {"get_s64l", [](StringReader& r) { expect_eq(r.get_s64l(), 0x0706050403020100); }, 0, 8},
      {"get_f32b", [](StringReader& r) { expect_eq(r.get_f32b(), 1.0f); }, 0x10, 4},
      {"get_f32l", [](StringReader& r) { expect_eq(r.get_f32l(), 1.0f); }, 0x14, 4},
      {"get_f64b", [](StringReader& r) { expect_eq(r.get_f64b(), 1.0); }, 0x18, 8},
      {"get_f64l", [](StringReader& r) { expect_eq(r.get_f64l(), 1.0); }, 0x20, 8},
      {"pget_u8", [](StringReader& r) { expect_eq(r.pget_u8(4), 0x04); }, 0, 0},
      {"pget_s8", [](StringReader& r) { expect_eq(r.pget_s8(4), 0x04); }, 0, 0},
      {"pget_u16b", [](StringReader& r) { expect_eq(r.pget_u16b(4), 0x0405); }, 0, 0},
      {"pget_u16l", [](StringReader& r) { expect_eq(r.pget_u16l(4), 0x0504); }, 0, 0},
      {"pget_s16b", [](StringReader& r) { expect_eq(r.pget_s16b(4), 0x0405); }, 0, 0},
      {"pget_s16l", [](StringReader& r) { expect_eq(r.pget_s16l(4), 0x0504); }, 0, 0},
      {"pget_u24b", [](StringReader& r) { expect_eq(r.pget_u24b(4), 0x040506); }, 0, 0},
      {"pget_u24l", [](StringReader& r) { expect_eq(r.pget_u24l(4), 0x060504); }, 0, 0},
      {"pget_s24b", [](StringReader& r) { expect_eq(r.pget_s24b(4), 0x040506); }, 0, 0},
      {"pget_s24l", [](StringReader& r) { expect_eq(r.pget_s24l(4), 0x060504); }, 0, 0},
      {"pget_u32b", [](StringReader& r) { expect_eq(r.pget_u32b(4), 0x04050607); }, 0, 0},
      {"pget_u32l", [](StringReader& r) { expect_eq(r.pget_u32l(4), 0x07060504); }, 0, 0},
      {"pget_s32b", [](StringReader& r) { expect_eq(r.pget_s32b(4), 0x04050607); }, 0, 0},
      {"pget_s32l", [](StringReader& r) { expect_eq(r.pget_s32l(4), 0x07060504); }, 0, 0},
      {"pget_u48b", [](StringReader& r) { expect_eq(r.pget_u48b(4), 0x040506070809); }, 0, 0},
      {"pget_u48l", [](StringReader& r) { expect_eq(r.pget_u48l(4), 0x090807060504); }, 0, 0},
      {"pget_s48b", [](StringReader& r) { expect_eq(r.pget_s48b(4), 0x040506070809); }, 0, 0},
      {"pget_s48l", [](StringReader& r) { expect_eq(r.pget_s48l(4), 0x090807060504); }, 0, 0},
      {"pget_u64b", [](StringReader& r) { expect_eq(r.pget_u64b(4), 0x0405060708090A0B); }, 0, 0},
      {"pget_u64l", [](StringReader& r) { expect_eq(r.pget_u64l(4), 0x0B0A090807060504); }, 0, 0},
      {"pget_s64b", [](StringReader& r) { expect_eq(r.pget_s64b(4), 0x0405060708090A0B); }, 0, 0},
      {"pget_s64l", [](StringReader& r) { expect_eq(r.pget_s64l(4), 0x0B0A090807060504); }, 0, 0},
      {"pget_f32b", [](StringReader& r) { expect_eq(r.pget_f32b(0x10), 1.0f); }, 0, 0},
      {"pget_f32l", [](StringReader& r) { expect_eq(r.pget_f32l(0x14), 1.0f); }, 0, 0},
      {"pget_f64b", [](StringReader& r) { expect_eq(r.pget_f64b(0x18), 1.0); }, 0, 0},
      {"pget_f64l", [](StringReader& r) { expect_eq(r.pget_f64l(0x20), 1.0); }, 0, 0},
  });

  for (const auto& get_test_case : get_test_cases) {
    fprintf(stderr, "---- %s\n", get_test_case.name);
    r.go(get_test_case.start_offset);
    get_test_case.fn(r);
    expect_eq(r.where(), get_test_case.start_offset + get_test_case.expected_advance);
  }

  fprintf(stderr, "---- get_cstr/pget_cstr\n");
  r.go(0x3A);
  expect_eq(r.get_cstr(), "and this is a cstring");
  expect(r.eof());
  expect_eq(r.pget_cstr(0x3A), "and this is a cstring");
}

int main(int, char**) {

  fprintf(stderr, "-- starts_with\n");
  expect(starts_with("abcdef", "abc"));
  expect(starts_with("abcdef", "abcdef"));
  expect(!starts_with("abcdef", "abcdefg"));
  expect(!starts_with("abcdef", "abd"));
  expect(!starts_with("abcdef", "dbc"));

  fprintf(stderr, "-- ends_with\n");
  expect(ends_with("abcdef", "def"));
  expect(ends_with("abcdef", "abcdef"));
  expect(!ends_with("abcdef", "gabcdef"));
  expect(!ends_with("abcdef", "ded"));
  expect(!ends_with("abcdef", "fef"));

  {
    fprintf(stderr, "-- str_replace_all\n");
    expect_eq("", str_replace_all(string(""), "def", "xyz"));
    expect_eq("abcdef", str_replace_all(string("abcdef"), "efg", "xyz"));
    expect_eq("abcxyz", str_replace_all(string("abcdef"), "def", "xyz"));
    expect_eq("abcxyzabc", str_replace_all(string("abcdefabc"), "def", "xyz"));
    expect_eq("abcxyzabcxyz", str_replace_all(string("abcdefabcdef"), "def", "xyz"));
    expect_eq("abcxyzabcxyzabc", str_replace_all(string("abcdefabcdefabc"), "def", "xyz"));
    expect_eq("xyzabcxyzabcxyzabc", str_replace_all(string("defabcdefabcdefabc"), "def", "xyz"));
  }

  {
    fprintf(stderr, "-- strip_trailing_zeroes\n");
    string s1("abcdef", 6);
    strip_trailing_zeroes(s1);
    expect_eq(s1, "abcdef");
    string s2("abcdef\0", 7);
    strip_trailing_zeroes(s2);
    expect_eq(s2, "abcdef");
    string s3("abcdef\0\0\0\0\0", 11);
    strip_trailing_zeroes(s3);
    expect_eq(s3, "abcdef");
    string s4("", 0);
    strip_trailing_zeroes(s4);
    expect_eq(s4, "");
    string s5("\0\0\0\0\0", 5);
    strip_trailing_zeroes(s5);
    expect_eq(s5, "");
  }

  {
    fprintf(stderr, "-- strip_trailing_whitespace\n");
    string s1 = "abcdef";
    strip_trailing_whitespace(s1);
    expect_eq(s1, "abcdef");
    string s2 = "abcdef\r\n";
    strip_trailing_whitespace(s2);
    expect_eq(s2, "abcdef");
    string s3 = "abc\tdef  \r\n";
    strip_trailing_whitespace(s3);
    expect_eq(s3, "abc\tdef");
    string s4 = "";
    strip_trailing_whitespace(s4);
    expect_eq(s4, "");
    string s5 = "   \t\r\n  ";
    strip_trailing_whitespace(s5);
    expect_eq(s5, "");
  }

  {
    fprintf(stderr, "-- strip_whitespace\n");

    string s = "abcdef";
    strip_whitespace(s);
    expect_eq(s, "abcdef");

    s = "abcdef\r\n";
    strip_whitespace(s);
    expect_eq(s, "abcdef");

    s = "  \nabc\tdef";
    strip_whitespace(s);
    expect_eq(s, "abc\tdef");

    s = "  \nabc\tdef  \r\n";
    strip_whitespace(s);
    expect_eq(s, "abc\tdef");

    s = "";
    strip_whitespace(s);
    expect_eq(s, "");

    s = "   \t\r\n  ";
    strip_whitespace(s);
    expect_eq(s, "");
  }

  {
    fprintf(stderr, "-- strip_multiline_comments\n");

    string s = "abc/*def*/ghi";
    strip_multiline_comments(s);
    expect_eq(s, "abcghi");

    s = "/*abc*/def\r\n";
    strip_multiline_comments(s);
    expect_eq(s, "def\r\n");

    s = "abc\n/*def\nghi*/\njkl";
    strip_multiline_comments(s);
    expect_eq(s, "abc\n\n\njkl");
  }

  {
    fprintf(stderr, "-- string_printf\n");
    string result = string_printf("%s %" PRIu64 " 0x%04hX", "lolz",
        static_cast<uint64_t>(1000), static_cast<uint16_t>(0x4F));
    fprintf(stderr, "%s\n", result.c_str());
    expect_eq("lolz 1000 0x004F", result);
  }

  {
    fprintf(stderr, "-- split\n");
    expect_eq(vector<string>({"12", "34", "567", "abc"}), split("12,34,567,abc", ','));
    expect_eq(vector<string>({"12", "34", "567", "", ""}), split("12,34,567,,", ','));
    expect_eq(vector<string>({""}), split("", ','));
    expect_eq(vector<string>({"a", "b", "c d e f"}), split("a b c d e f", ' ', 2));
  }

  {
    fprintf(stderr, "-- split_context\n");
    expect_eq(vector<string>({"12", "34", "567", "abc"}), split_context("12,34,567,abc", ','));
    expect_eq(vector<string>({"12", "34", "567", "", ""}), split_context("12,34,567,,", ','));
    expect_eq(vector<string>({""}), split_context("", ','));
    expect_eq(vector<string>({"a", "b", "c d e f"}), split_context("a b c d e f", ' ', 2));
    expect_eq(vector<string>({"12", "3(4,56)7", "ab[c,]d", "e{fg(h,),}"}),
        split_context("12,3(4,56)7,ab[c,]d,e{fg(h,),}", ','));
    expect_eq(vector<string>({"12", "(34,567)", "abc"}), split_context("12,(34,567),abc", ','));
    expect_eq(vector<string>({"12(,(34),567)", "abc"}), split_context("12(,(34),567),abc", ','));
    expect_eq(vector<string>({"12", "(,567)", "abc"}), split_context("12,(,567),abc", ','));
    expect_eq(vector<string>({"12", "(34,)", "abc"}), split_context("12,(34,),abc", ','));
    expect_eq(vector<string>({"12", "(,)", "abc"}), split_context("12,(,),abc", ','));
    expect_eq(vector<string>({"12", "(34,567),abc"}), split_context("12,(34,567),abc", ',', 1));
    expect_eq(vector<string>({"(12,34)", "567,abc"}), split_context("(12,34),567,abc", ',', 1));
  }

  {
    fprintf(stderr, "-- split_args\n");
    expect_eq(vector<string>(), split_args(""));
    expect_eq(vector<string>(), split_args("      "));
    expect_eq(vector<string>({"12", "34", "567", "abc"}), split_args("12 34 567 abc"));
    expect_eq(vector<string>({"12", "34 567", "abc"}), split_args("12 \'34 567\' abc"));
    expect_eq(vector<string>({"12", "34 567", "abc"}), split_args("12 \"34 567\" abc"));
    expect_eq(vector<string>({"12", "34 \'567", "abc"}), split_args("12 \'34 \\\'567\' abc"));
    expect_eq(vector<string>({"12", "34 \"567", "abc"}), split_args("12 \"34 \\\"567\" abc"));
    expect_eq(vector<string>({"12", "34 567", "abc"}), split_args("12 34\\ 567 abc"));
    expect_eq(vector<string>({"12", "34 567", "abc"}), split_args("   12 34\\ 567 abc   "));
    expect_eq(vector<string>({"12", "34 567", "abc", " "}), split_args("   12 34\\ 567 abc  \\   "));
  }

  fprintf(stderr, "-- skip_whitespace/skip_non_whitespace\n");
  expect_eq(0, skip_whitespace("1234", 0));
  expect_eq(2, skip_whitespace("  1234", 0));
  expect_eq(7, skip_whitespace("  \t\r\n  1234", 0));
  expect_eq(7, skip_whitespace("  \t\r\n  1234", 3));
  expect_eq(7, skip_whitespace("  \t\r\n  ", 0));
  expect_eq(7, skip_whitespace("  \t\r\n  ", 3));
  expect_eq(0, skip_whitespace(string("1234"), 0));
  expect_eq(2, skip_whitespace(string("  1234"), 0));
  expect_eq(7, skip_whitespace(string("  \t\r\n  1234"), 0));
  expect_eq(7, skip_whitespace(string("  \t\r\n  1234"), 3));
  expect_eq(7, skip_whitespace(string("  \t\r\n  "), 0));
  expect_eq(7, skip_whitespace(string("  \t\r\n  "), 3));
  expect_eq(4, skip_non_whitespace("1234 ", 0));
  expect_eq(4, skip_non_whitespace("1234 ", 2));
  expect_eq(4, skip_non_whitespace("1234\t", 0));
  expect_eq(4, skip_non_whitespace("1234\t", 2));
  expect_eq(4, skip_non_whitespace("1234\r", 0));
  expect_eq(4, skip_non_whitespace("1234\r", 2));
  expect_eq(4, skip_non_whitespace("1234\n", 0));
  expect_eq(4, skip_non_whitespace("1234\n", 2));
  expect_eq(4, skip_non_whitespace("1234", 0));
  expect_eq(4, skip_non_whitespace("1234", 2));
  expect_eq(4, skip_non_whitespace(string("1234 "), 0));
  expect_eq(4, skip_non_whitespace(string("1234 "), 2));
  expect_eq(4, skip_non_whitespace(string("1234\t"), 0));
  expect_eq(4, skip_non_whitespace(string("1234\t"), 2));
  expect_eq(4, skip_non_whitespace(string("1234\r"), 0));
  expect_eq(4, skip_non_whitespace(string("1234\r"), 2));
  expect_eq(4, skip_non_whitespace(string("1234\n"), 0));
  expect_eq(4, skip_non_whitespace(string("1234\n"), 2));
  expect_eq(4, skip_non_whitespace(string("1234"), 0));
  expect_eq(4, skip_non_whitespace(string("1234"), 2));

  fprintf(stderr, "-- skip_word\n");
  const char* sentence = "The quick brown fox jumped over the lazy dog.";
  vector<size_t> expected_offsets = {4, 10, 16, 20, 27, 32, 36, 41, 45};
  {
    vector<size_t> offsets;
    size_t offset = 0;
    while (sentence[offset]) {
      offset = skip_word(sentence, offset);
      offsets.emplace_back(offset);
    }
    expect_eq(expected_offsets, offsets);
  }
  {
    string sentence_str(sentence);
    vector<size_t> offsets;
    size_t offset = 0;
    while (sentence_str[offset]) {
      offset = skip_word(sentence_str, offset);
      offsets.emplace_back(offset);
    }
    expect_eq(expected_offsets, offsets);
  }

  fprintf(stderr, "-- parse_data_string/format_data_string with arbitrary data\n");
  {
    string input("/* omit 01 02 */ 03 ?04? $ ##30 $ ##127 ?\"dark\"? ###-1 \'cold\' %-1.667 %%-2.667");
    string expected_data(
        "\x03\x04" // 03 ?04?
        "\x00\x1E" // $ ##30 $
        "\x7F\x00" // ##127
        "\x64\x61\x72\x6B" // ?"dark"?
        "\xFF\xFF\xFF\xFF" // ###-1
        "\x63\x00\x6F\x00\x6C\x00\x64\x00" // 'cold'
        "\x42\x60\xD5\xBF" // %-1.667
        "\xBC\x74\x93\x18\x04\x56\x05\xC0", // %%-2.667
        34);
    string expected_mask("\xFF\x00\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 34);
    string output_mask;
    string output_data = parse_data_string(input, &output_mask);
    expect_eq(expected_data, output_data);
    expect_eq(expected_mask, output_mask);

    {
      string expected_formatted_input("03?04?001E7F00?6461726B?FFFFFFFF63006F006C0064004260D5BFBC749318045605C0");
      string formatted_input = format_data_string(output_data, &output_mask);
      expect_eq(expected_formatted_input, formatted_input);

      output_data = parse_data_string(input, &output_mask);
      expect_eq(expected_data, output_data);
      expect_eq(expected_mask, output_mask);
    }

    {
      string expected_formatted_input("0304001E7F006461726BFFFFFFFF63006F006C0064004260D5BFBC749318045605C0");
      string formatted_input = format_data_string(output_data, nullptr);
      expect_eq(expected_formatted_input, formatted_input);

      output_data = parse_data_string(input, &output_mask);
      expect_eq(expected_data, output_data);
      expect_eq(expected_mask, output_mask);
    }
  }

  fprintf(stderr, "-- parse_data_string/format_data_string with printable data\n");
  {
    string input("this is printable\nand it is sort of a haiku\nwith some control bytes\t\r\n");
    string expected_formatted("\"this is printable\\nand it is sort of a haiku\\nwith some control bytes\\t\\r\\n\"");
    string expected_formatted_hex("74686973206973207072696E7461626C650A616E6420697420697320736F7274206F662061206861696B750A7769746820736F6D6520636F6E74726F6C206279746573090D0A");
    string formatted = format_data_string(input);
    expect_eq(expected_formatted, formatted);
    expect_eq(input, parse_data_string(formatted));
    string formatted_hex = format_data_string(input, nullptr, FormatDataFlags::HEX_ONLY);
    expect_eq(expected_formatted_hex, formatted_hex);
    expect_eq(input, parse_data_string(formatted_hex));
  }

  fprintf(stderr, "-- parse_data_string/format_data_string with quotes in printable data\n");
  {
    string input("this string has \"some\" \'quotes\'.");
    string expected_formatted("\"this string has \\\"some\\\" \\\'quotes\\\'.\"");
    string formatted = format_data_string(input);
    expect_eq(expected_formatted, formatted);
    expect_eq(input, parse_data_string(formatted));
  }

  fprintf(stderr, "-- format_size\n");
  {
    expect_eq("0 bytes", format_size(0));
    expect_eq("1000 bytes", format_size(1000));
    expect_eq("1.50 KB", format_size(1536));
    expect_eq("1536 bytes (1.50 KB)", format_size(1536, true));
    expect_eq("1.00 GB", format_size(1073741824));
    expect_eq("1073741824 bytes (1.00 GB)", format_size(1073741824, true));
  }

  fprintf(stderr, "-- parse_size\n");
  {
    expect_eq(0, parse_size("0"));
    expect_eq(0, parse_size("0B"));
    expect_eq(0, parse_size("0 B"));
    expect_eq(0, parse_size("0 bytes"));
    expect_eq(1000, parse_size("1000 bytes"));
    expect_eq(1536, parse_size("1.5 KB"));
    expect_eq(3 * 1024 * 1024, parse_size("3 MB"));
  }

  fprintf(stderr, "-- escape_quotes\n");
  {
    expect_eq("", escape_quotes(""));
    expect_eq("omg hax", escape_quotes("omg hax"));
    expect_eq("\'omg\' \\\"hax\\\"", escape_quotes("\'omg\' \"hax\""));
  }

  fprintf(stderr, "-- escape_url\n");
  {
    expect_eq("", escape_url(""));
    expect_eq("omg%20hax", escape_url("omg hax"));
    expect_eq("slash/es", escape_url("slash/es"));
    expect_eq("slash%2Fes", escape_url("slash/es", true));
  }

  print_data_test();

  test_bit_reader();

  test_string_reader();

  // TODO: test string_vprintf
  // TODO: test log_level, set_log_level, log
  // TODO: test get_time_string
  // TODO: test string_for_error

  unlink("StringsTest-data");

  printf("StringsTest: all tests passed\n");
  return 0;
}
