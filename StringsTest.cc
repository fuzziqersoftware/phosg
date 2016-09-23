#include <assert.h>

#include "Strings.hh"

using namespace std;


int main(int argc, char** argv) {

  assert(starts_with("abcdef", "abc"));
  assert(starts_with("abcdef", "abcdef"));
  assert(!starts_with("abcdef", "abcdefg"));
  assert(!starts_with("abcdef", "abd"));
  assert(!starts_with("abcdef", "dbc"));

  assert(ends_with("abcdef", "def"));
  assert(ends_with("abcdef", "abcdef"));
  assert(!ends_with("abcdef", "gabcdef"));
  assert(!ends_with("abcdef", "ded"));
  assert(!ends_with("abcdef", "fef"));

  assert(string_printf("%s %llu %p", "lolz", 1000ULL, (void*)0x4F) == "lolz 1000 0x4f");

  {
    string in = "12,34,567,abc";
    vector<string> expected = {"12", "34", "567", "abc"};
    assert(split(in, ',') == expected);
  }

  {
    string in = "12,3(4,56)7,ab[c,]d,e{fg(h,),}";
    vector<string> expected = {"12", "3(4,56)7", "ab[c,]d", "e{fg(h,),}"};
    assert(split_context(in, ',') == expected);
  }

  assert(skip_whitespace("1234", 0) == 0);
  assert(skip_whitespace("  1234", 0) == 2);
  assert(skip_whitespace("  \t\r\n  1234", 0) == 7);
  assert(skip_whitespace("  \t\r\n  1234", 3) == 7);
  assert(skip_whitespace("  \t\r\n  ", 0) == 7);
  assert(skip_whitespace("  \t\r\n  ", 3) == 7);
  assert(skip_whitespace(string("1234"), 0) == 0);
  assert(skip_whitespace(string("  1234"), 0) == 2);
  assert(skip_whitespace(string("  \t\r\n  1234"), 0) == 7);
  assert(skip_whitespace(string("  \t\r\n  1234"), 3) == 7);
  assert(skip_whitespace(string("  \t\r\n  "), 0) == 7);
  assert(skip_whitespace(string("  \t\r\n  "), 3) == 7);

  assert(skip_non_whitespace("1234 ", 0) == 4);
  assert(skip_non_whitespace("1234 ", 2) == 4);
  assert(skip_non_whitespace("1234\t", 0) == 4);
  assert(skip_non_whitespace("1234\t", 2) == 4);
  assert(skip_non_whitespace("1234\r", 0) == 4);
  assert(skip_non_whitespace("1234\r", 2) == 4);
  assert(skip_non_whitespace("1234\n", 0) == 4);
  assert(skip_non_whitespace("1234\n", 2) == 4);
  assert(skip_non_whitespace("1234", 0) == 4);
  assert(skip_non_whitespace("1234", 2) == 4);
  assert(skip_non_whitespace(string("1234 "), 0) == 4);
  assert(skip_non_whitespace(string("1234 "), 2) == 4);
  assert(skip_non_whitespace(string("1234\t"), 0) == 4);
  assert(skip_non_whitespace(string("1234\t"), 2) == 4);
  assert(skip_non_whitespace(string("1234\r"), 0) == 4);
  assert(skip_non_whitespace(string("1234\r"), 2) == 4);
  assert(skip_non_whitespace(string("1234\n"), 0) == 4);
  assert(skip_non_whitespace(string("1234\n"), 2) == 4);
  assert(skip_non_whitespace(string("1234"), 0) == 4);
  assert(skip_non_whitespace(string("1234"), 2) == 4);

  const char* sentence = "The quick brown fox jumped over the lazy dog.";
  vector<size_t> expected_offsets = {4, 10, 16, 20, 27, 32, 36, 41, 45};
  {
    vector<size_t> offsets;
    size_t offset = 0;
    while (sentence[offset]) {
      offset = skip_word(sentence, offset);
      offsets.emplace_back(offset);
    }
    assert(offsets == expected_offsets);
  }
  {
    string sentence_str(sentence);
    vector<size_t> offsets;
    size_t offset = 0;
    while (sentence_str[offset]) {
      offset = skip_word(sentence_str, offset);
      offsets.emplace_back(offset);
    }
    assert(offsets == expected_offsets);
  }

  // TODO: test string_vprintf
  // TODO: test log_level, set_log_level, log
  // TODO: test string_for_error

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
