#include <sys/time.h>
#include <unistd.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Encoding.hh"
#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;


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

string string_printf(const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  string ret = string_vprintf(fmt, va);
  va_end(va);
  return ret;
}

string string_vprintf(const char* fmt, va_list va) {
  char* result = NULL;
  int length = vasprintf(&result, fmt, va);

  if (result == NULL) {
    throw bad_alloc();
  }

  string ret(result, length);
  free(result);
  return ret;
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

  static pid_t pid = 0;

  if (!pid) {
    pid = getpid();
  }

  char time_buffer[32];
  time_t now_secs = time(NULL);
  struct tm now_tm;
  localtime_r(&now_secs, &now_tm);
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
  fprintf(stderr, "%c %d %s - ", log_level_chars[level], pid, time_buffer);

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
      else if (s[i] == ',') {
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

string join(const vector<string>& items, const string& delim) {
  string ret;
  for (const string& item : items) {
    if (!ret.empty()) {
      ret += delim;
    }
    ret += item;
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
  char buffer[1024];
  strerror_r(error, buffer, sizeof(buffer));
  return string_printf("%d (%s)", error, buffer);
}

void print_color_escape(FILE* stream, TerminalFormat color, ...) {

  string fmt("\033");

  va_list va;
  va_start(va, color);
  do {
    fmt += (fmt[fmt.size() - 1] == '\033') ? '[' : ';';
    fmt += to_string((int)color);
    color = va_arg(va, TerminalFormat);
  } while (color != TerminalFormat::END);
  va_end(va);

  fmt += 'm';
  fwrite(fmt.data(), fmt.size(), 1, stream);
}

void print_data(FILE* stream, const void* _data, uint64_t size,
    uint64_t address, const void* _prev, bool use_color) {

  if (size == 0) {
    return;
  }

  // if color is disabled or no diff source is given, disable diffing
  const uint8_t* data = (const uint8_t*)_data;
  const uint8_t* prev = (const uint8_t*)(_prev ? _prev : _data);

  char data_ascii[20];
  char prev_ascii[20]; // actually only 16 is necessary but w/e

  // start_offset is how many blank spaces to print before the first byte
  int start_offset = address & 0x0F;
  address &= ~0x0F;
  size += start_offset;

  // if nonzero, print the address here (the loop won't do it for the 1st line)
  if (start_offset) {
    fprintf(stream, "%016llX | ", address);
  }

  // print initial spaces, if any
  unsigned long long x, y;
  for (x = 0; x < start_offset; x++) {
    fputs("   ", stream);
    data_ascii[x] = ' ';
    prev_ascii[x] = ' ';
  }

  // print the data
  for (; x < size; x++) {
    int line_offset = x & 0x0F;
    int data_offset = x - start_offset;
    data_ascii[line_offset] = data[data_offset];
    prev_ascii[line_offset] = prev[data_offset];

    // first byte on the line? then print the address
    if ((x & 0x0F) == 0) {
      fprintf(stream, "%016llX | ", address + x);
    }

    // print the byte itself
    if (use_color && (prev[data_offset] != data[data_offset])) {
      print_color_escape(stream, TerminalFormat::BOLD, TerminalFormat::FG_RED,
          TerminalFormat::END);
    }
    fprintf(stream, "%02X ", data[data_offset]);
    if (use_color && (prev[data_offset] != data[data_offset])) {
      print_color_escape(stream, TerminalFormat::NORMAL, TerminalFormat::END);
    }

    // last byte on the line? then print the ascii view and a \n
    if ((x & 0x0F) == 0x0F) {
      fputs("| ", stream);

      for (y = 0; y < 16; y++) {
        if (use_color && (prev_ascii[y] != data_ascii[y])) {
          print_color_escape(stream, TerminalFormat::BOLD, TerminalFormat::FG_RED,
              TerminalFormat::END);
        }

        if ((data_ascii[y] < 0x20) || (data_ascii[y] >= 0x7F)) {
          if (use_color) {
            print_color_escape(stream, TerminalFormat::INVERSE,
                TerminalFormat::END);
          }
          fputc(' ', stream);
          if (use_color) {
            print_color_escape(stream, TerminalFormat::NORMAL,
                TerminalFormat::END);
          }
        } else {
          fputc(data_ascii[y], stream);
        }

        if (use_color && (prev_ascii[y] != data_ascii[y])) {
          print_color_escape(stream, TerminalFormat::NORMAL,
              TerminalFormat::END);
        }
      }

      fputc('\n', stream);
    }
  }

  // if the last line is a partial line, print the remaining ascii chars
  if (x & 0x0F) {
    for (y = x; y & 0x0F; y++) {
      fprintf(stream, "   ");
    }
    fprintf(stream, "| ");

    for (y = 0; y < (x & 0x0F); y++) {
      if (use_color && (prev_ascii[y] != data_ascii[y])) {
        print_color_escape(stream, TerminalFormat::FG_RED, TerminalFormat::BOLD,
            TerminalFormat::END);
      }

      if ((data_ascii[y] < 0x20) || (data_ascii[y] == 0x7F)) {
        if (use_color) {
          print_color_escape(stream, TerminalFormat::INVERSE,
              TerminalFormat::END);
        }
        putc(' ', stream);
        if (use_color) {
          print_color_escape(stream, TerminalFormat::NORMAL,
              TerminalFormat::END);
        }
      } else {
        putc(data_ascii[y], stream);
      }

      if (use_color && (prev_ascii[y] != data_ascii[y])) {
        print_color_escape(stream, TerminalFormat::NORMAL, TerminalFormat::END);
      }
    }
    putc('\n', stream);
  }
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

  string ret;
  bool mask_enabled = true;
  size_t x;
  for (x = 0; x < data.size(); x++) {
    if (mask && ((bool)mask->at(x) != mask_enabled)) {
      mask_enabled = !mask_enabled;
      ret += '?';
    }
    ret += string_printf("%02X", (uint8_t)data[x]);
  }
  return ret;
}

string format_time(struct timeval* tv) {
  struct timeval local_tv;
  if (!tv) {
    tv = &local_tv;
    gettimeofday(tv, NULL);
  }

  struct tm cooked;
  localtime_r(&tv->tv_sec, &cooked);

  static const char* monthnames[] = {"January", "February", "March", "April",
      "May", "June", "July", "August", "September", "October", "November",
      "December"};

  return string_printf("%u %s %4u %02u:%02u:%02u.%03u", cooked.tm_mday,
     monthnames[cooked.tm_mon], cooked.tm_year + 1900,
     cooked.tm_hour, cooked.tm_min, cooked.tm_sec,
     tv->tv_usec / 1000);
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
    return string_printf("%llu bytes", size);
  }
  if (include_bytes) {
    if (size < MB_SIZE) {
      return string_printf("%llu bytes (%.02f KB)", size, (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return string_printf("%llu bytes (%.02f MB)", size, (float)size / MB_SIZE);
    }
    if (size < TB_SIZE) {
      return string_printf("%llu bytes (%.02f GB)", size, (float)size / GB_SIZE);
    }
    if (size < PB_SIZE) {
      return string_printf("%llu bytes (%.02f TB)", size, (float)size / TB_SIZE);
    }
    if (size < EB_SIZE) {
      return string_printf("%llu bytes (%.02f PB)", size, (float)size / PB_SIZE);
    }
    return string_printf("%llu bytes (%.02f EB)", size, (float)size / EB_SIZE);
  } else {
    if (size < MB_SIZE) {
      return string_printf("%.02f KB", (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return string_printf("%.02f MB", (float)size / MB_SIZE);
    }
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
  }
}
