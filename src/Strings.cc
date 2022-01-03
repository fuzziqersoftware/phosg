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

#ifdef PHOSG_WINDOWS
#include <windows.h>

#define PRIX8 "hhX"
#define PRIX64 "llX"
#endif

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

void strip_trailing_zeroes(string& s) {
  size_t index = s.find_last_not_of('\0');
  if (index != string::npos) {
    s.resize(index + 1);
  } else if (!s.empty() && s[0] == '\0') {
    s.resize(0); // String is entirely zeroes
  }
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

#ifdef PHOSG_WINDOWS
static int vasprintf(char** out, const char *fmt, va_list va) {
  int len = _vscprintf(fmt, va);
  if (len < 0) {
    return len;
  }

  char* s = reinterpret_cast<char*>(malloc(len + 1));
  if (!s) {
    return -1;
  }

  int r = vsprintf_s(s, len + 1, fmt, va);
  if (r < 0) {
    free(s);
  } else {
    *out = s;
  }
  return r;
}
#endif

string string_vprintf(const char* fmt, va_list va) {
  char* result = nullptr;
  int length = vasprintf(&result, fmt, va);

  if (result == nullptr) {
    throw bad_alloc();
  }

  string ret(result, length);
  free(result);
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
    written = vswprintf(const_cast<wchar_t*>(result.data()), result.size(), fmt, va);
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

static int current_log_level = INFO;

int log_level() {
  return current_log_level;
}

void set_log_level(int new_level) {
  current_log_level = new_level;
}

static const vector<char> log_level_chars({
  'D', 'I', 'W', 'E',
});

void log(int level, const char* fmt, ...) {
  if (level < current_log_level) {
    return;
  }

  char time_buffer[32];
  time_t now_secs = time(nullptr);
  struct tm now_tm;
  localtime_r(&now_secs, &now_tm);
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
#ifdef PHOSG_WINDOWS
  // don't include the pid on windows
  fprintf(stderr, "%c %s - ", log_level_chars[level], time_buffer);
#else
  fprintf(stderr, "%c %d %s - ", log_level_chars[level], getpid_cached(), time_buffer);
#endif

  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  putc('\n', stderr);
}

vector<string> split(const string& s, char delim) {
  vector<string> elems;
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

vector<wstring> split(const wstring& s, wchar_t delim) {
  vector<wstring> elems;
  wstringstream ss(s);
  wstring item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

vector<string> split_context(const string& s, char delim) {
  vector<string> elems;
  vector<char> paren_stack;

  size_t i, last_start = 0;
  for (i = 0; i < s.size(); i++) {
    if (paren_stack.size() > 0) {
      if (s[i] == paren_stack.back())
        paren_stack.pop_back();
    } else {
      if (s[i] == '(')
        paren_stack.push_back(')');
      else if (s[i] == '[')
        paren_stack.push_back(']');
      else if (s[i] == '{')
        paren_stack.push_back('}');
      else if (s[i] == '<')
        paren_stack.push_back('>');
      else if (s[i] == delim) {
        elems.push_back(s.substr(last_start, i - last_start));
        last_start = i + 1;
      }
    }
  }

  if (i >= last_start) {
    elems.push_back(s.substr(last_start, i));
  }

  if (paren_stack.size()) {
    throw runtime_error("Unbalanced parenthesis in split");
  }

  return elems;
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
#ifndef PHOSG_WINDOWS
  strerror_r(error, buffer, sizeof(buffer));
#else
  strerror_s(buffer, sizeof(buffer), error);
#endif
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

void print_data(FILE* stream, const void* _data, uint64_t size,
    uint64_t start_address, const void* _prev, uint64_t flags) {

  if (size == 0) {
    return;
  }

  uint64_t end_address = start_address + size;

  bool use_color = flags & PrintDataFlags::UseColor;
  bool print_ascii = flags & PrintDataFlags::PrintAscii;
  bool print_float = flags & PrintDataFlags::PrintFloat;
  bool print_double = flags & PrintDataFlags::PrintDouble;
  bool reverse_endian = flags & PrintDataFlags::ReverseEndian;
  bool collapse_zero_lines = flags & PrintDataFlags::CollapseZeroLines;

  // if color is disabled or no diff source is given, disable diffing
  const uint8_t* data = (const uint8_t*)_data;
  const uint8_t* prev = (const uint8_t*)(_prev ? _prev : _data);

  // print the data
  for (uint64_t line_start_address = start_address & (~0x0F);
       line_start_address < end_address;
       line_start_address += 0x10) {
    uint64_t line_end_address = line_start_address + 0x10;
    if (collapse_zero_lines && (line_start_address > start_address) && (line_end_address <= end_address) &&
        !memcmp(&data[line_start_address - start_address], "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16)) {
      continue;
    }

    fprintf(stream, "%016" PRIX64 " |", line_start_address);

    // print the hex view
    {
      uint64_t address = line_start_address;
      for (; (address < start_address) && (address < line_end_address); address++) {
        fputs("   ", stream);
      }
      for (; (address < end_address) && (address < line_end_address); address++) {
        uint8_t current_value = data[address - start_address];
        uint8_t previous_value = prev[address - start_address];

        RedBoldTerminalGuard g(stream, use_color && (previous_value != current_value));
        fprintf(stream, " %02" PRIX8, current_value);
      }
      for (; address < line_end_address; address++) {
        fputs("   ", stream);
      }
    }

    // print the ascii view
    if (print_ascii) {
      fputs(" | ", stream);

      uint64_t address = line_start_address;
      for (; (address < start_address) && (address < line_end_address); address++) {
        fputc(' ', stream);
      }
      for (; (address < end_address) && (address < line_end_address); address++) {
        uint8_t current_value = data[address - start_address];
        uint8_t previous_value = prev[address - start_address];

        RedBoldTerminalGuard g1(stream, use_color && (previous_value != current_value));
        if ((current_value < 0x20) || (current_value >= 0x7F)) {
          InverseTerminalGuard g2(stream, use_color);
          fputc(' ', stream);
        } else {
          fputc(current_value, stream);
        }
      }
      for (; address < line_end_address; address++) {
        fputc(' ', stream);
      }
    }

    // print the float view
    if (print_float) {
      fputs(" |", stream);

      uint64_t address = line_start_address;
      for (; (address < start_address) && (address < line_end_address); address += sizeof(float)) {
        fputs("             ", stream);
      }
      for (; (address + sizeof(float) <= end_address) && (address < line_end_address); address += sizeof(float)) {
        float current_value = reverse_endian ?
            bswap32f(*(uint32_t*)(&data[address - start_address])) :
            *(float*)(&data[address - start_address]);
        float previous_value = reverse_endian ?
            bswap32f(*(uint32_t*)(&prev[address - start_address])) :
            *(float*)(&prev[address - start_address]);

        RedBoldTerminalGuard g1(stream, use_color && (previous_value != current_value));
        fprintf(stream, " %12.5g", current_value);
      }
      for (; address < line_end_address; address += sizeof(float)) {
        fputs("             ", stream);
      }
    }

    // print the double view
    if (print_double) {
      fputs(" |", stream);

      uint64_t address = line_start_address;
      for (; (address < start_address) && (address < line_end_address); address += sizeof(double)) {
        fputs("             ", stream);
      }
      for (; (address + sizeof(double) <= end_address) && (address < line_end_address); address += sizeof(double)) {
        double current_value = reverse_endian ?
            bswap64f(*(uint64_t*)(&data[address - start_address])) :
            *(double*)(&data[address - start_address]);
        double previous_value = reverse_endian ?
            bswap64f(*(uint64_t*)(&prev[address - start_address])) :
            *(double*)(&prev[address - start_address]);

        RedBoldTerminalGuard g1(stream, use_color && (previous_value != current_value));
        fprintf(stream, " %12.5lg", current_value);
      }
      for (; address < line_end_address; address += sizeof(double)) {
        fputs("             ", stream);
      }
    }

    // done with this line
    fputc('\n', stream);
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

  return string_printf("%u %s %4u %02u:%02u:%02u.%03u", cooked.tm_mday,
     monthnames[cooked.tm_mon], cooked.tm_year + 1900,
     cooked.tm_hour, cooked.tm_min, cooked.tm_sec,
     tv->tv_usec / 1000);
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
    return string_printf("%hhu:%s", minutes,
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
    return string_printf("%hhu:%02hhu:%s", hours, minutes,
        ((seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;

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
    return string_printf("%" PRIu64 ":%02hhu:%02hhu:%s", days, hours,
        minutes, ((seconds_str.at(1) == '.') ? "0" : "")) + seconds_str;
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
}

bool StringReader::eof() const {
  return (this->offset >= this->length);
}

string StringReader::all() const {
  return string(reinterpret_cast<const char*>(this->data), this->length);
}

StringReader StringReader::sub(size_t offset, size_t size) const {
  if (offset + size > this->length) {
    throw out_of_range("sub-reader extends beyond end of data");
  }
  return StringReader(reinterpret_cast<const char*>(this->data) + offset, size);
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

size_t StringReader::read_into(void* data, size_t size, bool advance) {
  size_t ret = this->pread_into(this->offset, data, size);
  if (ret && advance) {
    this->offset += ret;
  }
  return ret;
}

void StringReader::readx_into(void* data, size_t size, bool advance) {
  this->preadx_into(this->offset, data, size);
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
  if ((offset >= this->length) || (offset + size > this->length)) {
    throw out_of_range("not enough data to read");
  }
  return string(reinterpret_cast<const char*>(this->data + offset), size);
}

size_t StringReader::pread_into(size_t offset, void* data, size_t size) const {
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

void StringReader::preadx_into(size_t offset, void* data, size_t size) const {
  if ((offset >= this->length) || (offset + size > this->length)) {
    throw out_of_range("not enough data to read");
  }
  memcpy(data, this->data + offset, size);
}

uint8_t StringReader::get_u8(bool advance) {
  uint8_t ret = this->pget_u8(this->offset);
  if (advance) {
    this->offset++;
  }
  return ret;
}

int8_t StringReader::get_s8(bool advance) {
  int8_t ret = this->pget_s8(this->offset);
  if (advance) {
    this->offset++;
  }
  return ret;
}

uint16_t StringReader::get_u16(bool advance) {
  uint16_t ret = this->pget_u16(this->offset);
  if (advance) {
    this->offset += 2;
  }
  return ret;
}

int16_t StringReader::get_s16(bool advance) {
  int16_t ret = this->pget_s16(this->offset);
  if (advance) {
    this->offset += 2;
  }
  return ret;
}

uint32_t StringReader::get_u24(bool advance) {
  uint32_t ret = this->pget_u24(this->offset);
  if (advance) {
    this->offset += 3;
  }
  return ret;
}

int32_t StringReader::get_s24(bool advance) {
  uint32_t x = this->get_u24(advance);
  if (x & 0x00800000) {
    return x | 0xFF000000;
  }
  return x;
}

uint32_t StringReader::get_u32(bool advance) {
  uint32_t ret = this->pget_u32(this->offset);
  if (advance) {
    this->offset += 4;
  }
  return ret;
}

int32_t StringReader::get_s32(bool advance) {
  int32_t ret = this->pget_s32(this->offset);
  if (advance) {
    this->offset += 4;
  }
  return ret;
}

uint64_t StringReader::get_u48(bool advance) {
  uint64_t ret = this->pget_u48(this->offset);
  if (advance) {
    this->offset += 6;
  }
  return ret;
}

int64_t StringReader::get_s48(bool advance) {
  uint32_t x = this->get_u48(advance);
  if (x & 0x0000800000000000) {
    return x | 0xFFFF000000000000;
  }
  return x;
}

uint64_t StringReader::get_u64(bool advance) {
  uint64_t ret = this->pget_u64(this->offset);
  if (advance) {
    this->offset += 8;
  }
  return ret;
}

int64_t StringReader::get_s64(bool advance) {
  int64_t ret = this->pget_s64(this->offset);
  if (advance) {
    this->offset += 8;
  }
  return ret;
}

float StringReader::get_f32(bool advance) {
  float ret = this->pget_f32(this->offset);
  if (advance) {
    this->offset += 4;
  }
  return ret;
}

double StringReader::get_f64(bool advance) {
  double ret = this->pget_f64(this->offset);
  if (advance) {
    this->offset += 8;
  }
  return ret;
}

uint16_t StringReader::get_u16r(bool advance) {
  return bswap16(this->get_u16(advance));
}

int16_t StringReader::get_s16r(bool advance) {
  return bswap16(this->get_s16(advance));
}

uint32_t StringReader::get_u24r(bool advance) {
  return bswap24(this->get_u24(advance));
}

int32_t StringReader::get_s24r(bool advance) {
  return bswap24s(this->get_s24(advance));
}

uint32_t StringReader::get_u32r(bool advance) {
  return bswap32(this->get_u32(advance));
}

int32_t StringReader::get_s32r(bool advance) {
  return bswap32(this->get_s32(advance));
}

uint64_t StringReader::get_u48r(bool advance) {
  return bswap48(this->get_u48(advance));
}

int64_t StringReader::get_s48r(bool advance) {
  return bswap48s(this->get_s48(advance));
}

uint64_t StringReader::get_u64r(bool advance) {
  return bswap64(this->get_u64(advance));
}

int64_t StringReader::get_s64r(bool advance) {
  return bswap64(this->get_s64(advance));
}

float StringReader::get_f32r(bool advance) {
  return bswap32f(this->get_u32(advance));
}

double StringReader::get_f64r(bool advance) {
  return bswap64f(this->get_u64(advance));
}

uint8_t StringReader::pget_u8(size_t offset) const {
  if (offset >= this->length) {
    throw out_of_range("end of string");
  }
  return static_cast<uint8_t>(this->data[offset]);
}

int8_t StringReader::pget_s8(size_t offset) const {
  if (offset >= this->length) {
    throw out_of_range("end of string");
  }
  return static_cast<int8_t>(this->data[offset]);
}

uint16_t StringReader::pget_u16(size_t offset) const {
  if (offset >= this->length - 1) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const uint16_t*>(this->data + offset);
}

int16_t StringReader::pget_s16(size_t offset) const {
  if (offset >= this->length - 1) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const int16_t*>(this->data + offset);
}

uint32_t StringReader::pget_u24(size_t offset) const {
  if (offset >= this->length - 2) {
    throw out_of_range("end of string");
  }
  uint32_t ret = this->data[offset] | (this->data[offset + 1] << 8) | (this->data[offset + 2] << 16);
  return ret;
}

int32_t StringReader::pget_s24(size_t offset) const {
  uint32_t x = this->pget_u24(offset);
  if (x & 0x00800000) {
    return x | 0xFF000000;
  }
  return x;
}

uint32_t StringReader::pget_u32(size_t offset) const {
  if (offset >= this->length - 3) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const uint32_t*>(this->data + offset);
}

int32_t StringReader::pget_s32(size_t offset) const {
  if (offset >= this->length - 3) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const int32_t*>(this->data + offset);
}

uint64_t StringReader::pget_u48(size_t offset) const {
  if (offset >= this->length - 5) {
    throw out_of_range("end of string");
  }
  uint64_t ret = static_cast<uint64_t>(this->data[offset]) |
      (static_cast<uint64_t>(this->data[offset + 1]) << 8) |
      (static_cast<uint64_t>(this->data[offset + 2]) << 16) |
      (static_cast<uint64_t>(this->data[offset + 3]) << 24) |
      (static_cast<uint64_t>(this->data[offset + 4]) << 32) |
      (static_cast<uint64_t>(this->data[offset + 5]) << 40);
  return ret;
}

int64_t StringReader::pget_s48(size_t offset) const {
  uint32_t x = this->pget_u48(offset);
  if (x & 0x0000800000000000) {
    return x | 0xFFFF000000000000;
  }
  return x;
}

uint64_t StringReader::pget_u64(size_t offset) const {
  if (offset >= this->length - 7) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const uint64_t*>(this->data + offset);
}

int64_t StringReader::pget_s64(size_t offset) const {
  if (offset >= this->length - 7) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const int64_t*>(this->data + offset);
}

float StringReader::pget_f32(size_t offset) const {
  if (offset >= this->length - 3) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const float*>(this->data + offset);
}

double StringReader::pget_f64(size_t offset) const {
  if (offset >= this->length - 7) {
    throw out_of_range("end of string");
  }
  return *reinterpret_cast<const double*>(this->data + offset);
}

uint16_t StringReader::pget_u16r(size_t offset) const {
  return bswap16(this->pget_u16(offset));
}

int16_t StringReader::pget_s16r(size_t offset) const {
  return bswap16(this->pget_s16(offset));
}

uint32_t StringReader::pget_u24r(size_t offset) const {
  return bswap24(this->pget_u24(offset));
}

int32_t StringReader::pget_s24r(size_t offset) const {
  return bswap24s(this->pget_s24(offset));
}

uint32_t StringReader::pget_u32r(size_t offset) const {
  return bswap32(this->pget_u32(offset));
}

int32_t StringReader::pget_s32r(size_t offset) const {
  return bswap32(this->pget_s32(offset));
}

uint64_t StringReader::pget_u48r(size_t offset) const {
  return bswap48(this->pget_u48(offset));
}

int64_t StringReader::pget_s48r(size_t offset) const {
  return bswap48s(this->pget_s48(offset));
}

uint64_t StringReader::pget_u64r(size_t offset) const {
  return bswap64(this->pget_u64(offset));
}

int64_t StringReader::pget_s64r(size_t offset) const {
  return bswap64(this->pget_s64(offset));
}

float StringReader::pget_f32r(size_t offset) const {
  return bswap32f(this->pget_u32(offset));
}

double StringReader::pget_f64r(size_t offset) const {
  return bswap64f(this->pget_u64(offset));
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

void StringWriter::write(const std::string& data) {
  this->data.append(data);
}

void StringWriter::put_u8(uint8_t v) {
  this->data.append(1, static_cast<char>(v));
}

void StringWriter::put_s8(int8_t v) {
  this->data.append(1, static_cast<char>(v));
}

void StringWriter::put_u16(uint16_t v) {
  this->data.append(reinterpret_cast<char*>(&v), sizeof(v));
}

void StringWriter::put_s16(int16_t v) {
  this->data.append(reinterpret_cast<char*>(&v), sizeof(v));
}

void StringWriter::put_u24(uint32_t v) {
  this->data.append(reinterpret_cast<char*>(&v), 3);
}

void StringWriter::put_s24(int32_t v) {
  this->data.append(reinterpret_cast<char*>(&v), 3);
}

void StringWriter::put_u32(uint32_t v) {
  this->data.append(reinterpret_cast<char*>(&v), sizeof(v));
}

void StringWriter::put_s32(int32_t v) {
  this->data.append(reinterpret_cast<char*>(&v), sizeof(v));
}

void StringWriter::put_u48(uint64_t v) {
  this->data.append(reinterpret_cast<char*>(&v), 6);
}

void StringWriter::put_s48(int64_t v) {
  this->data.append(reinterpret_cast<char*>(&v), 6);
}

void StringWriter::put_u64(uint64_t v) {
  this->data.append(reinterpret_cast<char*>(&v), sizeof(v));
}

void StringWriter::put_s64(int64_t v) {
  this->data.append(reinterpret_cast<char*>(&v), sizeof(v));
}


void StringWriter::put_u16r(uint16_t v) {
  this->put_u16(bswap16(v));
}

void StringWriter::put_s16r(int16_t v) {
  this->put_s16(bswap16(v));
}

void StringWriter::put_u24r(uint32_t v) {
  this->put_u24(bswap24(v));
}

void StringWriter::put_s24r(int32_t v) {
  this->put_s24(bswap24(v));
}

void StringWriter::put_u32r(uint32_t v) {
  this->put_u32(bswap32(v));
}

void StringWriter::put_s32r(int32_t v) {
  this->put_s32(bswap32(v));
}

void StringWriter::put_u48r(uint64_t v) {
  this->put_u48(bswap48(v));
}

void StringWriter::put_s48r(int64_t v) {
  this->put_s48(bswap48(v));
}

void StringWriter::put_u64r(uint64_t v) {
  this->put_u64(bswap64(v));
}

void StringWriter::put_s64r(int64_t v) {
  this->put_s64(bswap64(v));
}
