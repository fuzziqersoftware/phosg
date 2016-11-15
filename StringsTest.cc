#include <assert.h>
#include <sys/time.h>

#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;


int main(int argc, char** argv) {

  expect(starts_with("abcdef", "abc"));
  expect(starts_with("abcdef", "abcdef"));
  expect(!starts_with("abcdef", "abcdefg"));
  expect(!starts_with("abcdef", "abd"));
  expect(!starts_with("abcdef", "dbc"));

  expect(ends_with("abcdef", "def"));
  expect(ends_with("abcdef", "abcdef"));
  expect(!ends_with("abcdef", "gabcdef"));
  expect(!ends_with("abcdef", "ded"));
  expect(!ends_with("abcdef", "fef"));

  expect_eq("lolz 1000 0x4f", string_printf("%s %llu %p", "lolz", 1000ULL, (void*)0x4F));

  {
    string in = "12,34,567,abc";
    vector<string> expected = {"12", "34", "567", "abc"};
    expect_eq(expected, split(in, ','));
  }

  {
    string in = "12,3(4,56)7,ab[c,]d,e{fg(h,),}";
    vector<string> expected = {"12", "3(4,56)7", "ab[c,]d", "e{fg(h,),}"};
    expect_eq(expected, split_context(in, ','));
  }

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

  {
    expect_eq("0 bytes", format_size(0));
    expect_eq("1000 bytes", format_size(1000));
    expect_eq("1.50 KB", format_size(1536));
    expect_eq("1536 bytes (1.50 KB)", format_size(1536, true));
    expect_eq("1.00 GB", format_size(1073741824));
    expect_eq("1073741824 bytes (1.00 GB)", format_size(1073741824, true));
  }

  // TODO: test string_vprintf
  // TODO: test log_level, set_log_level, log
  // TODO: test get_time_string
  // TODO: test string_for_error

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
