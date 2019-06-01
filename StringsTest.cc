#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <assert.h>
#include <sys/time.h>

#include "Filesystem.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;


void print_data_test_case(const string& expected_output, const string& data,
    uint64_t address = 0, const string& prev = "",
    uint64_t flags = PrintDataFlags::PrintAscii) {

  // osx doesn't have fmemopen, so we just write to a file because I'm too lazy
  // to use funopen()
  {
    auto f = fopen_unique("StringsTest-data", "w");
    print_data(f.get(), data.data(), data.size(), address,
        prev.empty() ? NULL : prev.data(), flags);
  }
  string output_data = load_file("StringsTest-data");

  if (expected_output != output_data) {
    fputs("expected:\n", stderr);
    fwrite(expected_output.data(), 1, expected_output.size(), stderr);
    fputs("actual:\n", stderr);
    fwrite(output_data.data(), 1, output_data.size(), stderr);
  }

  expect_eq(expected_output, output_data);
}

void print_data_test() {

  // basic
  fprintf(stderr, "-- [print_data] basic\n");
  print_data_test_case("\
0000000000000000 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10));
  print_data_test_case("\
0000000000000000 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n\
0000000000000010 | 61 62 63 64 65 66 67 68 69                      | abcdefghi       \n",
string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x61\x62\x63\x64\x65\x66\x67\x68\x69", 0x19));

  // with address
  fprintf(stderr, "-- [print_data] address\n");
  print_data_test_case("\
3FFF3039AEC14EE0 | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |                 \n",
string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10),
0x3FFF3039AEC14EE0);
  print_data_test_case("\
3FFF3039AEC14EE0 |          00 01 02 03 04 05 06 07 08 09 0A 0B 0C |                 \n\
3FFF3039AEC14EF0 | 0D 0E 0F                                        |                 \n",
string("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 0x10),
0x3FFF3039AEC14EE3);
  print_data_test_case("\
3FFF3039AEC14EE0 |          61 63 65                               |    ace          \n",
"ace", 0x3FFF3039AEC14EE3);

  // without ascii
  fprintf(stderr, "-- [print_data] no ascii\n");
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
float_data, 0x0000000107B50FEC, "", PrintDataFlags::PrintAscii);
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |      -237.43            0       6.2612            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0       318.43       2.9893             \n",
float_data, 0x0000000107B50FEC, "", PrintDataFlags::PrintFloat);
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                  |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 | Vom      [ @     |      -237.43            0       6.2612            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |     n7 C>Q?@     |            0       318.43       2.9893             \n",
float_data, 0x0000000107B50FEC, "", PrintDataFlags::PrintAscii | PrintDataFlags::PrintFloat);

  // reverse-endian floats
  print_data_test_case("\
0000000107B50FE0 |                                     00 00 00 00 |                                                   0\n\
0000000107B50FF0 | 56 6F 6D C3 00 00 00 00 A5 5B C8 40 00 00 00 00 |   6.5814e+13            0  -1.9063e-16            0\n\
0000000107B51000 | 00 00 00 00 6E 37 9F 43 3E 51 3F 40             |            0   1.4207e+28      0.20434             \n",
float_data, 0x0000000107B50FEC, "", PrintDataFlags::PrintFloat | PrintDataFlags::ReverseEndian);

  // float alignment
  print_data_test_case("\
0000000000000000 |    00 00 00 00 00 00 00 00                      |                         0                          \n",
string("\0\0\0\0\0\0\0\0", 8), 1, "", PrintDataFlags::PrintFloat);

  // doubles
  print_data_test_case("\
0000000000000000 | 3F F0 00 00 00 00 00 00 41 FC 80 AC EA 2C AA 40 |  3.0387e-319       3350.5\n\
0000000000000010 | 40 00 00 00 00 00 00 00                         |   3.162e-322             \n",
string("\x3F\xF0\0\0\0\0\0\0\x41\xFC\x80\xAC\xEA\x2C\xAA\x40\x40\0\0\0\0\0\0\0", 0x18),
0, "", PrintDataFlags::PrintDouble | PrintDataFlags::PrintDouble);
  print_data_test_case("\
0000000000000000 | 3F F0 00 00 00 00 00 00 41 FC 80 AC EA 2C AA 40 |            1   7.6511e+09\n\
0000000000000010 | 40 00 00 00 00 00 00 00                         |            2             \n",
string("\x3F\xF0\0\0\0\0\0\0\x41\xFC\x80\xAC\xEA\x2C\xAA\x40\x40\0\0\0\0\0\0\0", 0x18),
0, "", PrintDataFlags::PrintDouble | PrintDataFlags::ReverseEndian);

  // reverse-endian doubles
  print_data_test_case("\
0000000000000000 | 3F F0 00 00 00 00 00 00 41 FC 80 AC EA 2C AA 40 | ?       A    , @ |            1   7.6511e+09\n\
0000000000000010 | 40 00 00 00 00 00 00 00                         | @                |            2             \n",
string("\x3F\xF0\0\0\0\0\0\0\x41\xFC\x80\xAC\xEA\x2C\xAA\x40\x40\0\0\0\0\0\0\0", 0x18),
0, "", PrintDataFlags::PrintDouble | PrintDataFlags::PrintAscii | PrintDataFlags::ReverseEndian);

  // double alignment
  print_data_test_case("\
0000000000000000 |    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |                         0\n\
0000000000000010 | 00                                              |                          \n",
string("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16), 1, "", PrintDataFlags::PrintDouble);

  // TODO: test diffing
}


int main(int argc, char** argv) {

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
    fprintf(stderr, "-- string_printf\n");
    string result = string_printf("%s %llu 0x%04hX", "lolz", 1000ULL, 0x4F);
    fprintf(stderr, "%s\n", result.c_str());
    expect_eq("lolz 1000 0x004F", result);
  }

  {
    fprintf(stderr, "-- split\n");
    string in = "12,34,567,abc";
    vector<string> expected = {"12", "34", "567", "abc"};
    expect_eq(expected, split(in, ','));
  }

  {
    fprintf(stderr, "-- split_context\n");
    string in = "12,3(4,56)7,ab[c,]d,e{fg(h,),}";
    vector<string> expected = {"12", "3(4,56)7", "ab[c,]d", "e{fg(h,),}"};
    expect_eq(expected, split_context(in, ','));
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

  fprintf(stderr, "-- parse_data_string/format_data_string\n");
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
      string formatted_input = format_data_string(output_data, NULL);
      expect_eq(expected_formatted_input, formatted_input);

      output_data = parse_data_string(input, &output_mask);
      expect_eq(expected_data, output_data);
      expect_eq(expected_mask, output_mask);
    }
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

  print_data_test();

  // TODO: test string_vprintf
  // TODO: test log_level, set_log_level, log
  // TODO: test get_time_string
  // TODO: test string_for_error

  unlink("StringsTest-data");

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}

