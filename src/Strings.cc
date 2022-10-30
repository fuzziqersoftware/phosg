#include "Strings.hh"

#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Platform.hh"

#include "Encoding.hh"
#include "Filesystem.hh"
#include "Process.hh"

using namespace std;


unique_ptr<void, void (*)(void*)> malloc_unique(size_t size) {
  return unique_ptr<void, void (*)(void*)>(malloc(size), free);
}


bool starts_with(const string& s, const string& start) {
  if (s.length() >= start.length()) {
    return (0 == s.compare(0, start.length(), start));
  }
  return false;
}

bool ends_with(const string& s, const string& end) {
  if (s.length() >= end.length()) {
    return (0 == s.compare(s.length() - end.length(), end.length(), end));
  }
  return false;
}

std::string toupper(const std::string& s) {
  std::string ret;
  ret.reserve(s.size());
  for (char ch : s) {
    ret.push_back(toupper(ch));
  }
  return ret;
}

std::string tolower(const std::string& s) {
  std::string ret;
  ret.reserve(s.size());
  for (char ch : s) {
    ret.push_back(tolower(ch));
  }
  return ret;
}

string escape_quotes(const string& s) {
  string ret;
  for (size_t x = 0; x < s.size(); x++) {
    char ch = s[x];
    if (ch == '\"') {
      ret += "\\\"";
    } else if (ch < 0x20 || ch > 0x7E) {
      ret += string_printf("\\x%02X", static_cast<uint8_t>(ch));
    } else {
      ret += ch;
    }
  }
  return ret;
}

string escape_url(const string& s, bool escape_slash) {
  string ret;
  for (char ch : s) {
    if (isalnum(ch) || (ch == '-') || (ch == '_') || (ch == '.') ||
        (ch == '~') || (ch == '=') || (ch == '&') || (!escape_slash && (ch == '/'))) {
      ret += ch;
    } else {
      ret += string_printf("%%%02hhX", ch);
    }
  }
  return ret;
}

string string_printf(const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  string ret = string_vprintf(fmt, va);
  va_end(va);
  return ret;
}

wstring wstring_printf(const wchar_t* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  wstring ret = wstring_vprintf(fmt, va);
  va_end(va);
  return ret;
}

string string_vprintf(const char* fmt, va_list va) {
#ifndef PHOSG_WINDOWS
  char* result = nullptr;
  int length = vasprintf(&result, fmt, va);

  if (result == nullptr) {
    throw bad_alloc();
  }

  string ret(result, length);
  free(result);

#else
  string ret(0x400, '\0');
  int size = vsnprintf(ret.data(), ret.size(), fmt, va);
  // TODO: We probably can handle this case more gracefully. Really, we should
  // just restart va, resize the string, and call vsnprintf again, but I'm too
  // lazy to test/verify that this works (or that va doesn't need restarting)
  if (size > 0x400) {
    throw logic_error("size too long");
  }
  ret.resize(size);

#endif
  return ret;
}

wstring wstring_vprintf(const wchar_t* fmt, va_list va) {
  // TODO: use open_wmemstream when it's available on mac os
  wstring result;
  result.resize(wcslen(fmt) * 2); // silly guess

  ssize_t written = -1;
  while ((written < 0) || (written > static_cast<ssize_t>(result.size()))) {
    va_list tmp_va;
    va_copy(tmp_va, va);
    written = vswprintf(result.data(), result.size(), fmt, va);
    va_end(tmp_va);
  }
  result.resize(written);
  return result;
}

uint8_t value_for_hex_char(char x) {
  if (x >= '0' && x <= '9') {
    return x - '0';
  }
  if (x >= 'A' && x <= 'F') {
    return (x - 'A') + 0xA;
  }
  if (x >= 'a' && x <= 'f') {
    return (x - 'a') + 0xA;
  }
  throw out_of_range(string_printf("invalid hex char: %c", x));
}



static LogLevel current_log_level = LogLevel::INFO;

LogLevel log_level() {
  return current_log_level;
}

void set_log_level(LogLevel new_level) {
  current_log_level = new_level;
}

static const vector<char> log_level_chars({
  'D', 'I', 'W', 'E',
});

void print_log_prefix(FILE* stream, LogLevel level) {
  char time_buffer[32];
  time_t now_secs = time(nullptr);
  struct tm now_tm;
  localtime_r(&now_secs, &now_tm);
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
  char level_char = log_level_chars.at(static_cast<int>(level));
  fprintf(stream, "%c %d %s - ", level_char, getpid_cached(), time_buffer);
}

void log_debug_v(const char* fmt, va_list va)   { log_v<LogLevel::DEBUG>(fmt, va); }
void log_info_v(const char* fmt, va_list va)    { log_v<LogLevel::INFO>(fmt, va); }
void log_warning_v(const char* fmt, va_list va) { log_v<LogLevel::WARNING>(fmt, va); }
void log_error_v(const char* fmt, va_list va)   { log_v<LogLevel::ERROR>(fmt, va); }

#define LOG_HELPER_BODY(LEVEL) \
  if (should_log(LEVEL)) { \
    va_list va; \
    va_start(va, fmt); \
    log_v<LEVEL>(fmt, va); \
    va_end(va); \
  }

__attribute__((format(printf, 1, 2))) void log_debug(const char* fmt, ...)   { LOG_HELPER_BODY(LogLevel::DEBUG); }
__attribute__((format(printf, 1, 2))) void log_info(const char* fmt, ...)    { LOG_HELPER_BODY(LogLevel::INFO); }
__attribute__((format(printf, 1, 2))) void log_warning(const char* fmt, ...) { LOG_HELPER_BODY(LogLevel::WARNING); }
__attribute__((format(printf, 1, 2))) void log_error(const char* fmt, ...)   { LOG_HELPER_BODY(LogLevel::ERROR); }

#undef LOG_HELPER_BODY

PrefixedLogger::PrefixedLogger(const std::string& prefix, LogLevel min_level)
  : prefix(prefix), min_level(min_level) { }



vector<string> split(const string& s, char delim, size_t max_splits) {
  vector<string> ret;

  // Note: token_start_offset can be equal to s.size() if the string ends with
  // the delimiter character; in that case, we need to ensure we correctly
  // return an empty string at the end of ret.
  size_t token_start_offset = 0;
  while (token_start_offset <= s.size()) {
    size_t delim_offset = (max_splits && (ret.size() == max_splits))
        ? string::npos
        : s.find(delim, token_start_offset);
    if (delim_offset == string::npos) {
      ret.emplace_back(s.substr(token_start_offset));
      break;
    } else {
      ret.emplace_back(s.substr(token_start_offset, delim_offset - token_start_offset));
      token_start_offset = delim_offset + 1;
    }
  }
  return ret;
}

vector<wstring> split(const wstring& s, wchar_t delim, size_t max_splits) {
  vector<wstring> ret;

  // Note: token_start_offset can be equal to s.size() if the string ends with
  // the delimiter character; in that case, we need to ensure we correctly
  // return an empty string at the end of ret.
  size_t token_start_offset = 0;
  while (token_start_offset <= s.size()) {
    size_t delim_offset = (max_splits && (ret.size() == max_splits))
        ? string::npos
        : s.find(delim, token_start_offset);
    if (delim_offset == string::npos) {
      ret.emplace_back(s.substr(token_start_offset));
      break;
    } else {
      ret.emplace_back(s.substr(token_start_offset, delim_offset - token_start_offset));
      token_start_offset = delim_offset + 1;
    }
  }
  return ret;
}

vector<string> split_context(const string& s, char delim, size_t max_splits) {
  vector<string> ret;
  vector<char> paren_stack;

  size_t z, last_start = 0;
  for (z = 0; z < s.size(); z++) {
    if (paren_stack.size() > 0) {
      if (s[z] == paren_stack.back()) {
        paren_stack.pop_back();
      }
    } else {
      if (s[z] == '(') {
        paren_stack.push_back(')');
      } else if (s[z] == '[') {
        paren_stack.push_back(']');
      } else if (s[z] == '{') {
        paren_stack.push_back('}');
      } else if (s[z] == '<') {
        paren_stack.push_back('>');
      } else if ((s[z] == delim) && (!max_splits || (ret.size() < max_splits))) {
        ret.push_back(s.substr(last_start, z - last_start));
        last_start = z + 1;
      }
    }
  }

  if (z >= last_start) {
    ret.push_back(s.substr(last_start));
  }

  if (paren_stack.size()) {
    throw runtime_error("unbalanced parentheses in split_context");
  }

  return ret;
}

size_t skip_whitespace(const string& s, size_t offset) {
  while (offset < s.length() &&
      (s[offset] == ' ' || s[offset] == '\t' || s[offset] == '\r' || s[offset] == '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_whitespace(const char* s, size_t offset) {
  while (*s && (s[offset] == ' ' || s[offset] == '\t' || s[offset] == '\r' || s[offset] == '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_non_whitespace(const string& s, size_t offset) {
  while (offset < s.length() &&
      (s[offset] != ' ' && s[offset] != '\t' && s[offset] != '\r' && s[offset] != '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_non_whitespace(const char* s, size_t offset) {
  while (s[offset] && (s[offset] != ' ' && s[offset] != '\t' && s[offset] != '\r' && s[offset] != '\n')) {
    offset++;
  }
  return offset;
}

size_t skip_word(const string& s, size_t offset) {
  return skip_whitespace(s, skip_non_whitespace(s, offset));
}

size_t skip_word(const char* s, size_t offset) {
  return skip_whitespace(s, skip_non_whitespace(s, offset));
}

std::string string_for_error(int error) {
  char buffer[1024] = "Unknown error";
  strerror_r(error, buffer, sizeof(buffer));
  return string_printf("%d (%s)", error, buffer);
}

string vformat_color_escape(TerminalFormat color, va_list va) {
  string fmt("\033");

  do {
    fmt += (fmt[fmt.size() - 1] == '\033') ? '[' : ';';
    fmt += to_string((int)color);
    color = va_arg(va, TerminalFormat);
  } while (color != TerminalFormat::END);

  fmt += 'm';
  return fmt;
}

string format_color_escape(TerminalFormat color, ...) {
  va_list va;
  va_start(va, color);
  string ret = vformat_color_escape(color, va);
  va_end(va);
  return ret;
}

void print_color_escape(FILE* stream, TerminalFormat color, ...) {
  va_list va;
  va_start(va, color);
  string fmt = vformat_color_escape(color, va);
  va_end(va);
  fwrite(fmt.data(), fmt.size(), 1, stream);
}

void print_indent(FILE* stream, int indent_level) {
  for (; indent_level > 0; indent_level--) {
    fputc(' ', stream);
    fputc(' ', stream);
  }
}

// TODO: generalize these classes
class RedBoldTerminalGuard {
public:
  RedBoldTerminalGuard(FILE* f, bool active = true) : f(f), active(active) {
    if (this->active) {
      print_color_escape(this->f, TerminalFormat::BOLD, TerminalFormat::FG_RED,
          TerminalFormat::END);
    }
  }
  ~RedBoldTerminalGuard() {
    if (this->active) {
      print_color_escape(this->f, TerminalFormat::NORMAL, TerminalFormat::END);
    }
  }
private:
  FILE* f;
  bool active;
};

class InverseTerminalGuard {
public:
  InverseTerminalGuard(FILE* f, bool active = true) : f(f), active(active) {
    if (this->active) {
      print_color_escape(this->f, TerminalFormat::INVERSE, TerminalFormat::END);
    }
  }
  ~InverseTerminalGuard() {
    if (this->active) {
      print_color_escape(this->f, TerminalFormat::NORMAL, TerminalFormat::END);
    }
  }
private:
  FILE* f;
  bool active;
};

void print_data(
    FILE* stream,
    const struct iovec* iovs,
    size_t num_iovs,
    uint64_t start_address,
    const struct iovec* prev_iovs,
    size_t num_prev_iovs,
    uint64_t flags) {
  if (num_iovs == 0) {
    return;
  }

  size_t total_size = 0;
  for (size_t x = 0; x < num_iovs; x++) {
    total_size += iovs[x].iov_len;
  }
  if (total_size == 0) {
    return;
  }

  if (num_prev_iovs) {
    size_t total_prev_size = 0;
    for (size_t x = 0; x < num_prev_iovs; x++) {
      total_prev_size += prev_iovs[x].iov_len;
    }
    if (total_prev_size != total_size) {
      throw runtime_error("previous iovs given, but data size does not match");
    }
  }

  uint64_t end_address = start_address + total_size;

  int width_digits;
  if (flags & PrintDataFlags::OFFSET_8_BITS) {
    width_digits = 2;
  } else if (flags & PrintDataFlags::OFFSET_16_BITS) {
    width_digits = 4;
  } else if (flags & PrintDataFlags::OFFSET_32_BITS) {
    width_digits = 8;
  } else if (flags & PrintDataFlags::OFFSET_64_BITS) {
    width_digits = 16;
  } else if (end_address > 0x100000000) {
    width_digits = 16;
  } else if (end_address > 0x10000) {
    width_digits = 8;
  } else if (end_address > 0x100) {
    width_digits = 4;
  } else {
    width_digits = 2;
  }

  bool use_color;
  if (flags & PrintDataFlags::USE_COLOR) {
    use_color = true;
  } else if (flags & PrintDataFlags::DISABLE_COLOR) {
    use_color = false;
  } else {
    use_color = isatty(fileno(stream));
  }
  bool print_ascii = flags & PrintDataFlags::PRINT_ASCII;
  bool print_float = flags & PrintDataFlags::PRINT_FLOAT;
  bool print_double = flags & PrintDataFlags::PRINT_DOUBLE;
  bool reverse_endian = flags & PrintDataFlags::REVERSE_ENDIAN_FLOATS;
  bool big_endian = flags & PrintDataFlags::BIG_ENDIAN_FLOATS;
  bool little_endian = flags & PrintDataFlags::LITTLE_ENDIAN_FLOATS;
  bool collapse_zero_lines = flags & PrintDataFlags::COLLAPSE_ZERO_LINES;
  bool skip_separator = flags & PrintDataFlags::SKIP_SEPARATOR;

  // Reserve space for the current/previous line data
  uint8_t line_buf[0x10];
  memset(line_buf, 0, sizeof(line_buf));

  uint8_t prev_line_buf[0x10];
  uint8_t* prev_line_data = prev_line_buf;
  if (num_prev_iovs) {
    memset(prev_line_buf, 0, sizeof(prev_line_buf));
  } else {
    prev_line_data = line_buf;
  }

  size_t current_iov_index = 0;
  size_t current_iov_bytes = 0;
  size_t prev_iov_index = 0;
  size_t prev_iov_bytes = 0;
  for (uint64_t line_start_address = start_address & (~0x0F);
       line_start_address < end_address;
       line_start_address += 0x10) {

    // Figure out the boundaries of the current line
    uint64_t line_end_address = line_start_address + 0x10;
    uint8_t line_invalid_start_bytes = max<int64_t>(start_address - line_start_address, 0);
    uint8_t line_invalid_end_bytes = max<int64_t>(line_end_address - end_address, 0);
    uint8_t line_bytes = 0x10 - line_invalid_end_bytes - line_invalid_start_bytes;

    auto print_fields_column = [&]<typename LoadedDataT, typename StoredDataT>(
        const char* field_format,
        const char* blank_format) -> void {
      fputs(skip_separator ? " " : " |", stream);

      const auto* line_fields = reinterpret_cast<const StoredDataT*>(line_buf);
      const auto* prev_line_fields = reinterpret_cast<const StoredDataT*>(prev_line_data);
      uint8_t line_invalid_start_fields = (line_invalid_start_bytes + sizeof(StoredDataT) - 1) / sizeof(StoredDataT);
      uint8_t line_invalid_end_fields = (line_invalid_end_bytes + sizeof(StoredDataT) - 1) / sizeof(StoredDataT);

      constexpr size_t field_count = 0x10 / sizeof(StoredDataT);
      size_t x = 0;
      for (; x < line_invalid_start_fields; x++) {
        fputs(blank_format, stream);
      }
      for (; x < static_cast<size_t>(field_count - line_invalid_end_fields); x++) {
        LoadedDataT current_value = line_fields[x];
        LoadedDataT previous_value = prev_line_fields[x];

        RedBoldTerminalGuard g1(stream, use_color && (previous_value != current_value));
        fprintf(stream, field_format, current_value);
      }
      for (; x < field_count; x++) {
        fputs(blank_format, stream);
      }
    };

    // Read the current and previous data for this line
    for (size_t x = 0; x < line_bytes; x++) {
      while (current_iov_bytes >= iovs[current_iov_index].iov_len) {
        current_iov_bytes = 0;
        current_iov_index++;
        if (current_iov_index >= num_iovs) {
          throw logic_error("reads exceeded final iov");
        }
      }
      line_buf[x + line_invalid_start_bytes] = reinterpret_cast<const uint8_t*>(
          iovs[current_iov_index].iov_base)[current_iov_bytes];
      current_iov_bytes++;
    }
    if (num_prev_iovs) {
      for (size_t x = 0; x < line_bytes; x++) {
        while (prev_iov_bytes >= prev_iovs[prev_iov_index].iov_len) {
          prev_iov_bytes = 0;
          prev_iov_index++;
          if (prev_iov_index >= num_prev_iovs) {
            throw logic_error("reads exceeded final prev iov");
          }
        }
        prev_line_buf[x + line_invalid_start_bytes] = reinterpret_cast<const uint8_t*>(
            prev_iovs[prev_iov_index].iov_base)[prev_iov_bytes];
        prev_iov_bytes++;
      }
    }

    if (collapse_zero_lines && (line_start_address > start_address) && (line_end_address < end_address) &&
        !memcmp(line_buf, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) &&
        !memcmp(prev_line_data, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16)) {
      continue;
    }

    fprintf(stream, skip_separator ? "%0*" PRIX64 : "%0*" PRIX64 " |",
        width_digits, line_start_address);

    {
      size_t x = 0;
      for (; x < line_invalid_start_bytes; x++) {
        fputs("   ", stream);
      }
      for (; x < static_cast<size_t>(0x10 - line_invalid_end_bytes); x++) {
        uint8_t current_value = line_buf[x];
        uint8_t previous_value = prev_line_data[x];

        RedBoldTerminalGuard g(stream, use_color && (previous_value != current_value));
        fprintf(stream, " %02" PRIX8, current_value);
      }
      for (; x < 0x10; x++) {
        fputs("   ", stream);
      }
    }

    if (print_ascii) {
      fputs(skip_separator ? " " : " | ", stream);

      size_t x = 0;
      for (; x < line_invalid_start_bytes; x++) {
        fputc(' ', stream);
      }
      for (; x < static_cast<size_t>(0x10 - line_invalid_end_bytes); x++) {
        uint8_t current_value = line_buf[x];
        uint8_t previous_value = prev_line_data[x];

        RedBoldTerminalGuard g1(stream, use_color && (previous_value != current_value));
        if ((current_value < 0x20) || (current_value >= 0x7F)) {
          InverseTerminalGuard g2(stream, use_color);
          fputc(' ', stream);
        } else {
          fputc(current_value, stream);
        }
      }
      for (; x < 0x10; x++) {
        fputc(' ', stream);
      }
    }

    if (print_float) {
      if (reverse_endian) {
        print_fields_column.operator()<float, re_float>(" %12.5g", "             ");
      } else if (big_endian) {
        print_fields_column.operator()<float, be_float>(" %12.5g", "             ");
      } else if (little_endian) {
        print_fields_column.operator()<float, le_float>(" %12.5g", "             ");
      } else {
        print_fields_column.operator()<float, float>(" %12.5g", "             ");
      }
    }
    if (print_double) {
      if (reverse_endian) {
        print_fields_column.operator()<double, re_double>(" %12.5lg", "             ");
      } else if (big_endian) {
        print_fields_column.operator()<double, be_double>(" %12.5lg", "             ");
      } else if (little_endian) {
        print_fields_column.operator()<double, le_double>(" %12.5lg", "             ");
      } else {
        print_fields_column.operator()<double, double>(" %12.5lg", "             ");
      }
    }

    fputc('\n', stream);
  }
}

void print_data(
    FILE* stream,
    const vector<struct iovec>& iovs,
    uint64_t start_address,
    const vector<struct iovec>* prev_iovs,
    uint64_t flags) {
  if (prev_iovs) {
    print_data(stream, iovs.data(), iovs.size(), start_address,
        prev_iovs->data(), prev_iovs->size(), flags);
  } else {
    print_data(stream, iovs.data(), iovs.size(), start_address, nullptr, 0,
        flags);
  }
}

void print_data(FILE* stream, const void* data, uint64_t size,
    uint64_t start_address, const void* prev, uint64_t flags) {
  iovec iov;
  iov.iov_base = const_cast<void*>(data);
  iov.iov_len = size;
  if (prev) {
    iovec prev_iov;
    prev_iov.iov_base = const_cast<void*>(prev);
    prev_iov.iov_len = size;
    print_data(stream, &iov, 1, start_address, &prev_iov, 1, flags);
  } else {
    print_data(stream, &iov, 1, start_address, nullptr, 0, flags);
  }
}

void print_data(FILE* stream, const std::string& data, uint64_t address,
    const void* prev, uint64_t flags) {
  print_data(stream, data.data(), data.size(), address, prev, flags);
}



static inline void add_mask_bits(string* mask, bool mask_enabled, size_t num_bytes) {
  if (!mask) {
    return;
  }
  mask->append(num_bytes, mask_enabled ? '\xFF' : '\x00');
}

string parse_data_string(const string& s, string* mask) {

  const char* in = s.c_str();

  string data;
  if (mask) {
    mask->clear();
  }

  uint8_t chr = 0;
  bool reading_string = false, reading_unicode_string = false,
       reading_comment = false, reading_multiline_comment = false,
       reading_high_nybble = true, reading_filename = false,
       inverse_endian = false, mask_enabled = true;
  string filename;
  while (in[0]) {
    bool read_nybble = 0;

    // if between // and a newline, don't write to output buffer
    if (reading_comment) {
      if (in[0] == '\n') {
        reading_comment = false;
      }
      in++;

    // if between /* and */, don't write to output buffer
    } else if (reading_multiline_comment) {
      if ((in[0] == '*') && (in[1] == '/')) {
        reading_multiline_comment = 0;
        in += 2;
      } else {
        in++;
      }

    // if between quotes, read bytes to output buffer, unescaping where needed
    } else if (reading_string) {
      if (in[0] == '\"') {
        reading_string = 0;
        in++;

      } else if (in[0] == '\\') { // unescape char after a backslash
        if (!in[1]) {
          return data;
        } else if (in[1] == 'n') {
          data += '\n';
        } else if (in[1] == 'r') {
          data += '\r';
        } else if (in[1] == 't') {
          data += '\t';
        } else {
          data += in[1];
        }
        add_mask_bits(mask, mask_enabled, 1);
        in += 2;

      } else {
        data += in[0];
        add_mask_bits(mask, mask_enabled, 1);
        in++;
      }

    // if between single quotes, word-expand bytes to output buffer, unescaping
    } else if (reading_unicode_string) {
      if (in[0] == '\'') {
        reading_unicode_string = 0;
        in++;

      } else if (in[0] == '\\') { // unescape char after a backslash
        int16_t value;
        if (!in[1]) {
          return data;
        } else if (in[1] == 'n') {
          value = '\n';
        } else if (in[1] == 'r') {
          value = '\r';
        } else if (in[1] == 't') {
          value = '\t';
        } else {
          value = in[1];
        }
        if (inverse_endian) {
          value = bswap16(value);
        }
        data.append((const char*)&value, 2);
        add_mask_bits(mask, mask_enabled, 2);
        in += 2;

      } else {
        int16_t value = in[0];
        if (inverse_endian) {
          bswap16(value);
        }
        data.append((const char*)&value, 2);
        add_mask_bits(mask, mask_enabled, 2);
        in++;
      }

    // if between <>, read a file name, then stick that file into the buffer
    } else if (reading_filename) {
      if (in[0] == '>') {
        // TODO: support <filename@offset:size> syntax
        reading_filename = 0;
        size_t pre_size = data.size();
        data += load_file(filename);
        add_mask_bits(mask, mask_enabled, data.size() - pre_size);

      } else {
        filename.append(1, in[0]);
      }
      in++;

    // ? inverts mask_enabled
    } else if (in[0] == '?') {
      mask_enabled = !mask_enabled;
      in++;

    // $ changes the endianness
    } else if (in[0] == '$') {
      inverse_endian = !inverse_endian;
      in++;

    // # signifies a decimal number
    } else if (in[0] == '#') { // 8-bit
      in++;
      if (in[0] == '#') { // 16-bit
        in++;
        if (in[0] == '#') { // 32-bit
          in++;
          if (in[0] == '#') { // 64-bit
            in++;
            uint64_t value = strtoull(in, const_cast<char**>(&in), 0);
            if (inverse_endian) {
              value = bswap64(value);
            }
            data.append((const char*)&value, 8);
            add_mask_bits(mask, mask_enabled, 8);

          } else {
            uint32_t value = strtoull(in, const_cast<char**>(&in), 0);
            if (inverse_endian) {
              value = bswap32(value);
            }
            data.append((const char*)&value, 4);
            add_mask_bits(mask, mask_enabled, 4);
          }

        } else {
          uint16_t value = strtoull(in, const_cast<char**>(&in), 0);
          if (inverse_endian) {
            value = bswap16(value);
          }
          data.append((const char*)&value, 2);
          add_mask_bits(mask, mask_enabled, 2);
        }

      } else {
        data.append(1, (char)strtoull(in, const_cast<char**>(&in), 0));
        add_mask_bits(mask, mask_enabled, 1);
      }

    // % is a float, %% is a double
    } else if (in[0] == '%') {
      in++;
      if (in[0] == '%') {
        in++;

        uint64_t value;
        *(double*)&value = strtod(in, const_cast<char**>(&in));
        if (inverse_endian) {
          value = bswap64(value);
        }
        data.append((const char*)&value, 8);
        add_mask_bits(mask, mask_enabled, 8);

      } else {
        uint32_t value;
        *(float*)&value = strtof(in, const_cast<char**>(&in));
        if (inverse_endian) {
          value = bswap32(value);
        }
        data.append((const char*)&value, 4);
        add_mask_bits(mask, mask_enabled, 4);
      }

    // anything else is a hex digit
    } else {
      if ((in[0] >= '0') && (in[0] <= '9')) {
        read_nybble = true;
        chr |= (in[0] - '0');

      } else if ((in[0] >= 'A') && (in[0] <= 'F')) {
        read_nybble = true;
        chr |= (in[0] - 'A' + 0x0A);

      } else if ((in[0] >= 'a') && (in[0] <= 'f')) {
        read_nybble = true;
        chr |= (in[0] - 'a' + 0x0A);

      } else if (in[0] == '\"') {
        reading_string = true;

      } else if (in[0] == '\'') {
        reading_unicode_string = 1;

      } else if ((in[0] == '/') && (in[1] == '/')) {
        reading_comment = 1;

      } else if ((in[0] == '/') && (in[1] == '*')) {
        reading_multiline_comment = 1;

      } else if (in[0] == '<') {
        reading_filename = 1;
        filename.clear();
      }
      in++;
    }

    if (read_nybble) {
      if (reading_high_nybble) {
        chr = chr << 4;
      } else {
        data += (char)chr;
        add_mask_bits(mask, mask_enabled, 1);
        chr = 0;
      }
      reading_high_nybble = !reading_high_nybble;
    }
  }
  return data;
}

string format_data_string(const string& data, const string* mask) {
  if (mask && (mask->size() != data.size())) {
    throw logic_error("data and mask sizes do not match");
  }
  return format_data_string(data.data(), data.size(), mask ? mask->data() : nullptr);
}

string format_data_string(const void* vdata, size_t size, const void* vmask) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);
  const uint8_t* mask = reinterpret_cast<const uint8_t*>(vmask);

  string ret;
  bool mask_enabled = true;
  size_t x;
  for (x = 0; x < size; x++) {
    if (mask && ((bool)mask[x] != mask_enabled)) {
      mask_enabled = !mask_enabled;
      ret += '?';
    }
    ret += string_printf("%02X", data[x]);
  }
  return ret;
}

string format_time(struct timeval* tv) {
  struct timeval local_tv;
  if (!tv) {
    tv = &local_tv;
    gettimeofday(tv, nullptr);
  }

  time_t sec = tv->tv_sec;
  struct tm cooked;
  localtime_r(&sec, &cooked);

  static const char* monthnames[] = {"January", "February", "March", "April",
      "May", "June", "July", "August", "September", "October", "November",
      "December"};

  return string_printf("%u %s %4u %02u:%02u:%02u.%03hu", cooked.tm_mday,
     monthnames[cooked.tm_mon], cooked.tm_year + 1900,
     cooked.tm_hour, cooked.tm_min, cooked.tm_sec,
     static_cast<uint16_t>(tv->tv_usec / 1000));
}

string format_duration(uint64_t usecs, int8_t subsecond_precision) {
  if (usecs < 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 5;
    }
    return string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs) / 1000000ULL);

  } else if (usecs < 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = (usecs < 10 * 1000000ULL) ? 5 : 4;
    }
    return string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs) / 1000000ULL);

  } else if (usecs < 60 * 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = (usecs < 10 * 60 * 1000000ULL) ? 2 : 1;
    }
    uint64_t minutes = usecs / (60 * 1000000ULL);
    uint64_t usecs_part = usecs - (minutes * 60 * 1000000ULL);
    string seconds_str = string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs_part) / 1000000ULL);
    return string_printf("%" PRIu64 ":%s", minutes,
        ((seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;

  } else if (usecs < 24 * 60 * 60 * 1000000ULL) {
    if (subsecond_precision < 0) {
      subsecond_precision = 0;
    }
    uint64_t hours = usecs / (60 * 60 * 1000000ULL);
    uint64_t minutes = (usecs / (60 * 1000000ULL)) % 60;
    uint64_t usecs_part = usecs
        - (hours * 60 * 60 * 1000000ULL)
        - (minutes * 60 * 1000000ULL);
    string seconds_str = string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs_part) / 1000000ULL);
    return string_printf("%" PRIu64 ":%02" PRIu64 ":%s", hours, minutes,
        ((seconds_str.size() > 1 && seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;

  } else {
    if (subsecond_precision < 0) {
      subsecond_precision = 0;
    }
    uint64_t days = usecs / (24 * 60 * 60 * 1000000ULL);
    uint64_t hours = (usecs / (60 * 60 * 1000000ULL)) % 24;
    uint64_t minutes = (usecs / (60 * 1000000ULL)) % 60;
    uint64_t usecs_part = usecs
        - (days * 24 * 60 * 60 * 1000000ULL)
        - (hours * 60 * 60 * 1000000ULL)
        - (minutes * 60 * 1000000ULL);
    string seconds_str = string_printf("%.*lf", subsecond_precision,
        static_cast<double>(usecs_part) / 1000000ULL);
    return string_printf("%" PRIu64 ":%02" PRIu64 ":%02" PRIu64 ":%s", days,
        hours, minutes, ((seconds_str.size() > 1 && seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;
  }
}

#define KB_SIZE 1024ULL
#define MB_SIZE (KB_SIZE * 1024ULL)
#define GB_SIZE (MB_SIZE * 1024ULL)
#define TB_SIZE (GB_SIZE * 1024ULL)
#define PB_SIZE (TB_SIZE * 1024ULL)
#define EB_SIZE (PB_SIZE * 1024ULL)
#define ZB_SIZE (EB_SIZE * 1024ULL)
#define YB_SIZE (ZB_SIZE * 1024ULL)
#define HB_SIZE (YB_SIZE * 1024ULL)

string format_size(size_t size, bool include_bytes) {

  if (size < KB_SIZE) {
    return string_printf("%zu bytes", size);
  }
  if (include_bytes) {
    if (size < MB_SIZE) {
      return string_printf("%zu bytes (%.02f KB)", size, (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return string_printf("%zu bytes (%.02f MB)", size, (float)size / MB_SIZE);
    }
#ifndef __x86_64__
    // size_t can only represent up to 4GB in a 32-bit environment
    return string_printf("%zu bytes (%.02f GB)", size, (float)size / GB_SIZE);
#else
    if (size < TB_SIZE) {
      return string_printf("%zu bytes (%.02f GB)", size, (float)size / GB_SIZE);
    }
    if (size < PB_SIZE) {
      return string_printf("%zu bytes (%.02f TB)", size, (float)size / TB_SIZE);
    }
    if (size < EB_SIZE) {
      return string_printf("%zu bytes (%.02f PB)", size, (float)size / PB_SIZE);
    }
    return string_printf("%zu bytes (%.02f EB)", size, (float)size / EB_SIZE);
#endif
  } else {
    if (size < MB_SIZE) {
      return string_printf("%.02f KB", (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return string_printf("%.02f MB", (float)size / MB_SIZE);
    }
#ifndef __x86_64__
    // size_t can only represent up to 4GB in a 32-bit environment
    return string_printf("%.02f GB", (float)size / GB_SIZE);
#else
    if (size < TB_SIZE) {
      return string_printf("%.02f GB", (float)size / GB_SIZE);
    }
    if (size < PB_SIZE) {
      return string_printf("%.02f TB", (float)size / TB_SIZE);
    }
    if (size < EB_SIZE) {
      return string_printf("%.02f PB", (float)size / PB_SIZE);
    }
    return string_printf("%.02f EB", (float)size / EB_SIZE);
#endif
  }
}

size_t parse_size(const char* str) {
  // input is like [0-9](\.[0-9]+)? *[KkMmGgTtPpEe]?[Bb]?
  // fortunately this can just be parsed left-to-right
  double fractional_part = 0.0;
  size_t integer_part = 0;
  size_t unit_scale = 1;
  for (; isdigit(*str); str++) {
    integer_part = integer_part * 10 + (*str - '0');
  }
  if (*str == '.') {
    str++;
    double factor = 0.1;
    for (; isdigit(*str); str++) {
      fractional_part += factor * (*str - '0');
      factor *= 0.1;
    }
  }
  for (; *str == ' '; str++) { }
  if (*str == 'K' || *str == 'k') {
    unit_scale = KB_SIZE;
  } else if (*str == 'M' || *str == 'm') {
    unit_scale = MB_SIZE;
  } else if (*str == 'G' || *str == 'g') {
    unit_scale = GB_SIZE;
  } else if (*str == 'T' || *str == 't') {
    unit_scale = TB_SIZE;
  } else if (*str == 'P' || *str == 'p') {
    unit_scale = PB_SIZE;
  } else if (*str == 'E' || *str == 'e') {
    unit_scale = EB_SIZE;
  }

  return integer_part * unit_scale + static_cast<size_t>(fractional_part * unit_scale);
}



BitReader::BitReader()
  : owned_data(nullptr), data(nullptr), length(0), offset(0) { }

BitReader::BitReader(shared_ptr<string> data, size_t offset) :
    owned_data(data), data(reinterpret_cast<const uint8_t*>(data->data())),
    length(data->size() * 8), offset(offset) { }

BitReader::BitReader(const void* data, size_t size, size_t offset) :
    data(reinterpret_cast<const uint8_t*>(data)), length(size), offset(offset) { }

BitReader::BitReader(const string& data, size_t offset) :
    BitReader(data.data(), data.size() * 8, offset) { }

size_t BitReader::where() const {
  return this->offset;
}

size_t BitReader::size() const {
  return this->length;
}

size_t BitReader::remaining() const {
  return this->length - this->offset;
}

void BitReader::truncate(size_t new_size) {
  if (this->length < new_size) {
    throw invalid_argument("BitReader contents cannot be extended");
  }
  this->length = new_size;
}

void BitReader::go(size_t offset) {
  this->offset = offset;
}

void BitReader::skip(size_t bits) {
  this->offset += bits;
}

bool BitReader::eof() const {
  return (this->offset >= this->length);
}

uint64_t BitReader::pread(size_t start_offset, uint8_t size) {
  if (size > 64) {
    throw logic_error("BitReader cannot return more than 64 bits at once");
  }

  uint64_t ret = 0;
  for (uint8_t ret_bits = 0; ret_bits < size; ret_bits++) {
    size_t bit_offset = start_offset + ret_bits;
    ret = (ret << 1) | ((this->data[bit_offset >> 3] >> (7 - (bit_offset & 7))) & 1);
  }
  return ret;
}

uint64_t BitReader::read(uint8_t size, bool advance) {
  uint64_t ret = this->pread(this->offset, size);
  if (advance) {
    this->offset += size;
  }
  return ret;
}



BitWriter::BitWriter() : last_byte_unset_bits(0) { }

size_t BitWriter::size() const {
  return this->data.size() * 8 - this->last_byte_unset_bits;
}

void BitWriter::reset() {
  this->data.clear();
  this->last_byte_unset_bits = 0;
}

void BitWriter::truncate(size_t size) {
  if (size > ((this->data.size() * 8) - this->last_byte_unset_bits)) {
    throw logic_error("cannot extend a BitWriter via truncate()");
  }
  this->data.resize((size + 7) / 8);
  this->last_byte_unset_bits = (8 - (size & 7)) & 7;
  // The if statement is important here (we can't just let the & become
  // degenerate) because this->data could now be empty
  if (this->last_byte_unset_bits) {
    this->data[this->data.size() - 1] &= (0xFF << this->last_byte_unset_bits);
  }
}

void BitWriter::write(bool v) {
  if (this->last_byte_unset_bits > 0) {
    this->last_byte_unset_bits--;
    if (v) {
      this->data[this->data.size() - 1] |= (1 << this->last_byte_unset_bits);
    }
  } else {
    this->data.push_back(v ? 0x80 : 0x00);
    this->last_byte_unset_bits = 7;
  }
}



StringReader::StringReader()
  : owned_data(nullptr), data(nullptr), length(0), offset(0) { }

StringReader::StringReader(shared_ptr<string> data, size_t offset) :
    owned_data(data), data(reinterpret_cast<const uint8_t*>(data->data())),
    length(data->size()), offset(offset) { }

StringReader::StringReader(const void* data, size_t size, size_t offset) :
    data(reinterpret_cast<const uint8_t*>(data)), length(size), offset(offset) { }

StringReader::StringReader(const string& data, size_t offset) :
    StringReader(data.data(), data.size(), offset) { }

size_t StringReader::where() const {
  return this->offset;
}

size_t StringReader::size() const {
  return this->length;
}

size_t StringReader::remaining() const {
  return this->length - this->offset;
}

void StringReader::truncate(size_t new_size) {
  if (this->length < new_size) {
    throw invalid_argument("StringReader contents cannot be extended");
  }
  this->length = new_size;
}

void StringReader::go(size_t offset) {
  this->offset = offset;
}

void StringReader::skip(size_t bytes) {
  this->offset += bytes;
  if (this->offset > this->length) {
    this->offset = this->length;
    throw out_of_range("skip beyond end of string");
  }
}

bool StringReader::eof() const {
  return (this->offset >= this->length);
}

string StringReader::all() const {
  return string(reinterpret_cast<const char*>(this->data), this->length);
}

StringReader StringReader::sub(size_t offset) const {
  if (offset > this->length) {
    return StringReader();
  }
  return StringReader(
      reinterpret_cast<const char*>(this->data) + offset,
      this->length - offset);
}

StringReader StringReader::sub(size_t offset, size_t size) const {
  if (offset >= this->length) {
    return StringReader();
  }
  if (offset + size > this->length) {
    return StringReader(
        reinterpret_cast<const char*>(this->data) + offset,
        this->length - offset);
  }
  return StringReader(reinterpret_cast<const char*>(this->data) + offset, size);
}

StringReader StringReader::subx(size_t offset) const {
  if (offset > this->length) {
    throw out_of_range("sub-reader begins beyond end of data");
  }
  return StringReader(
      reinterpret_cast<const char*>(this->data) + offset,
      this->length - offset);
}

StringReader StringReader::subx(size_t offset, size_t size) const {
  if (offset + size > this->length) {
    throw out_of_range("sub-reader begins or extends beyond end of data");
  }
  return StringReader(reinterpret_cast<const char*>(this->data) + offset, size);
}

BitReader StringReader::sub_bits(size_t offset) const {
  if (offset > this->length) {
    return BitReader();
  }
  return BitReader(
      reinterpret_cast<const char*>(this->data) + offset,
      (this->length - offset) * 8);
}

BitReader StringReader::sub_bits(size_t offset, size_t size) const {
  if (offset >= this->length) {
    return BitReader();
  }
  if (offset + size > this->length) {
    return BitReader(
        reinterpret_cast<const char*>(this->data) + offset,
        (this->length - offset) * 8);
  }
  return BitReader(reinterpret_cast<const char*>(this->data) + offset, size * 8);
}

BitReader StringReader::subx_bits(size_t offset) const {
  if (offset > this->length) {
    throw out_of_range("sub-reader begins beyond end of data");
  }
  return BitReader(
      reinterpret_cast<const char*>(this->data) + offset,
      (this->length - offset) * 8);
}

BitReader StringReader::subx_bits(size_t offset, size_t size) const {
  if (offset + size > this->length) {
    throw out_of_range("sub-reader begins or extends beyond end of data");
  }
  return BitReader(reinterpret_cast<const char*>(this->data) + offset, size * 8);
}

const char* StringReader::peek(size_t size) {
  if (this->offset + size <= this->length) {
    return reinterpret_cast<const char*>(this->data + this->offset);
  }
  throw out_of_range("not enough data to read");
}

string StringReader::read(size_t size, bool advance) {
  string ret = this->pread(this->offset, size);
  if (ret.size() && advance) {
    this->offset += ret.size();
  }
  return ret;
}

string StringReader::readx(size_t size, bool advance) {
  string ret = this->preadx(this->offset, size);
  if (advance) {
    this->offset += ret.size();
  }
  return ret;
}

size_t StringReader::read(void* data, size_t size, bool advance) {
  size_t ret = this->pread(this->offset, data, size);
  if (ret && advance) {
    this->offset += ret;
  }
  return ret;
}

void StringReader::readx(void* data, size_t size, bool advance) {
  this->preadx(this->offset, data, size);
  if (advance) {
    this->offset += size;
  }
}

string StringReader::pread(size_t offset, size_t size) const {
  if (offset >= this->length) {
    return string();
  }
  if (offset + size > this->length) {
    return string(reinterpret_cast<const char*>(this->data + offset), this->length - offset);
  }
  return string(reinterpret_cast<const char*>(this->data + offset), size);
}

string StringReader::preadx(size_t offset, size_t size) const {
  if (offset + size > this->length) {
    throw out_of_range("not enough data to read");
  }
  return string(reinterpret_cast<const char*>(this->data + offset), size);
}

size_t StringReader::pread(size_t offset, void* data, size_t size) const {
  if (offset >= this->length) {
    return 0;
  }

  size_t ret;
  if (offset + size > this->length) {
    memcpy(data, this->data + offset, this->length - offset);
    ret = this->length - offset;
  } else {
    memcpy(data, this->data + offset, size);
    ret = size;
  }
  return ret;
}

void StringReader::preadx(size_t offset, void* data, size_t size) const {
  if ((offset >= this->length) || (offset + size > this->length)) {
    throw out_of_range("not enough data to read");
  }
  memcpy(data, this->data + offset, size);
}

string StringReader::get_line(bool advance) {
  if (this->eof()) {
    throw out_of_range("end of string");
  }

  string ret;
  for (;;) {
    size_t ch_offset = this->offset + ret.size();
    if (ch_offset >= this->length) {
      break;
    }
    uint8_t ch = this->pget_s8(ch_offset);
    if (ch != '\n') {
      ret += ch;
    } else {
      break;
    }
  }
  if (advance) {
    this->offset += (ret.size() + 1);
  }
  if (ends_with(ret, "\r")) {
    ret.pop_back();
  }
  return ret;
}

string StringReader::get_cstr(bool advance) {
  string ret = this->pget_cstr(this->offset);
  if (advance) {
    this->offset += (ret.size() + 1);
  }
  return ret;
}

string StringReader::pget_cstr(size_t offset) const {
  string ret;
  for (;;) {
    uint8_t ch = this->pget_s8(offset + ret.size());
    if (ch != 0) {
      ret += ch;
    } else {
      break;
    }
  }
  return ret;
}



size_t StringWriter::size() const {
  return this->data.size();
}

void StringWriter::reset() {
  this->data.clear();
}

void StringWriter::write(const void* data, size_t size) {
  this->data.append(reinterpret_cast<const char*>(data), size);
}

void StringWriter::write(const std::string& data) {
  this->data.append(data);
}
