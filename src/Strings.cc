#include "Strings.hh"

#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <format>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Platform.hh"

#include "Encoding.hh"
#include "Filesystem.hh"
#include "Process.hh"

namespace phosg {

std::unique_ptr<void, void (*)(void*)> malloc_unique(size_t size) {
  return std::unique_ptr<void, void (*)(void*)>(malloc(size), free);
}

std::string toupper(std::string_view s) {
  std::string ret;
  ret.reserve(s.size());
  for (char ch : s) {
    ret.push_back(::toupper(ch));
  }
  return ret;
}

std::string tolower(std::string_view s) {
  std::string ret;
  ret.reserve(s.size());
  for (char ch : s) {
    ret.push_back(::tolower(ch));
  }
  return ret;
}

std::string str_replace_all(std::string_view s, const char* target, const char* replacement) {
  size_t target_size = strlen(target);
  size_t replacement_size = strlen(replacement);

  std::string ret;
  for (size_t read_offset = 0; read_offset < s.size();) {
    size_t find_offset = s.find(target, read_offset, target_size);
    if (find_offset == std::string::npos) {
      ret.append(s.data() + read_offset, s.size() - read_offset);
      read_offset = s.size();
    } else {
      ret.append(s.data() + read_offset, find_offset - read_offset);
      ret.append(replacement, replacement_size);
      read_offset = find_offset + target_size;
    }
  }
  return ret;
}

std::string escape_quotes(std::string_view s) {
  std::string ret;
  for (size_t x = 0; x < s.size(); x++) {
    char ch = s[x];
    if (ch == '\"') {
      ret += "\\\"";
    } else if (ch < 0x20 || ch > 0x7E) {
      ret += std::format("\\x{:02X}", static_cast<uint8_t>(ch));
    } else {
      ret += ch;
    }
  }
  return ret;
}

std::string escape_controls(std::string_view s, bool escape_non_ascii) {
  std::string ret;
  for (size_t x = 0; x < s.size(); x++) {
    char ch = s[x];
    if (ch == '\"') {
      ret += "\\\"";
    } else if (ch == '\'') {
      ret += "\\\'";
    } else if (ch == '\\') {
      ret += "\\\\";
    } else if (ch == '\t') {
      ret += "\\t";
    } else if (ch == '\r') {
      ret += "\\r";
    } else if (ch == '\n') {
      ret += "\\n";
    } else if (ch == '\f') {
      ret += "\\f";
    } else if (ch == '\b') {
      ret += "\\b";
    } else if (ch == '\a') {
      ret += "\\a";
    } else if (ch == '\v') {
      ret += "\\v";
    } else if (escape_non_ascii ? (ch < 0x20 || ch > 0x7E) : (!(ch & 0x80) && ((ch < 0x20) || ch == 0x7F))) {
      ret += std::format("\\x{:02X}", static_cast<uint8_t>(ch));
    } else {
      ret += ch;
    }
  }
  return ret;
}

std::string escape_url(std::string_view s, bool escape_slash) {
  std::string ret;
  for (char ch : s) {
    if (isalnum(ch) || (ch == '-') || (ch == '_') || (ch == '.') ||
        (ch == '~') || (ch == '=') || (ch == '&') || (!escape_slash && (ch == '/'))) {
      ret += ch;
    } else {
      ret += std::format("%{:02X}", ch);
    }
  }
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
  throw std::out_of_range(std::format("invalid hex char: {:c}", x));
}

template <>
LogLevel enum_for_name<LogLevel>(const char* name) {
  if (!strcmp(name, "USE_DEFAULT")) {
    return LogLevel::L_USE_DEFAULT;
  } else if (!strcmp(name, "DEBUG")) {
    return LogLevel::L_DEBUG;
  } else if (!strcmp(name, "INFO")) {
    return LogLevel::L_INFO;
  } else if (!strcmp(name, "WARNING")) {
    return LogLevel::L_WARNING;
  } else if (!strcmp(name, "ERROR")) {
    return LogLevel::L_ERROR;
  } else if (!strcmp(name, "DISABLED")) {
    return LogLevel::L_DISABLED;
  } else {
    throw std::invalid_argument("invalid LogLevel name");
  }
}

template <>
const char* name_for_enum<LogLevel>(LogLevel level) {
  switch (level) {
    case LogLevel::L_USE_DEFAULT:
      return "USE_DEFAULT";
    case LogLevel::L_DEBUG:
      return "DEBUG";
    case LogLevel::L_INFO:
      return "INFO";
    case LogLevel::L_WARNING:
      return "WARNING";
    case LogLevel::L_ERROR:
      return "ERROR";
    case LogLevel::L_DISABLED:
      return "DISABLED";
    default:
      throw std::invalid_argument("invalid LogLevel value");
  }
}

static LogLevel current_log_level = LogLevel::L_INFO;

LogLevel log_level() {
  return current_log_level;
}

void set_log_level(LogLevel new_level) {
  current_log_level = new_level;
}

static const std::vector<char> log_level_chars{'D', 'I', 'W', 'E'};

void print_log_prefix(FILE* stream, LogLevel level) {
  char time_buffer[32];
  time_t now_secs = time(nullptr);
  struct tm now_tm;
#ifndef PHOSG_WINDOWS
  localtime_r(&now_secs, &now_tm);
#else
  localtime_s(&now_tm, &now_secs);
#endif
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &now_tm);
  char level_char = log_level_chars.at(static_cast<int>(level));
  fwrite_fmt(stream, "{:c} {} {} - ", level_char, getpid_cached(), &time_buffer[0]);
}

PrefixedLogger::PrefixedLogger(std::string_view prefix, LogLevel min_level) : prefix(prefix), min_level(min_level) {}

PrefixedLogger PrefixedLogger::sub(std::string_view sub_prefix, LogLevel min_level) const {
  std::string new_prefix{this->prefix};
  new_prefix += sub_prefix;
  return PrefixedLogger(std::move(new_prefix), min_level == LogLevel::L_USE_DEFAULT ? this->min_level : min_level);
}

template <typename RetT, typename InT, typename CharT>
std::vector<RetT> split_inner(InT s, CharT delim, size_t max_splits) {
  std::vector<RetT> ret;

  // Note: token_start_offset can be equal to s.size() if the string ends with the delimiter character; in that case,
  // we need to ensure we correctly return an empty string at the end of ret.
  size_t token_start_offset = 0;
  while (token_start_offset <= s.size()) {
    size_t delim_offset = (max_splits && (ret.size() == max_splits)) ? RetT::npos : s.find(delim, token_start_offset);
    if (delim_offset == RetT::npos) {
      ret.emplace_back(s.substr(token_start_offset));
      break;
    } else {
      ret.emplace_back(s.substr(token_start_offset, delim_offset - token_start_offset));
      token_start_offset = delim_offset + 1;
    }
  }
  return ret;
}

std::vector<std::string> split(std::string_view s, char delim, size_t max_splits) {
  return split_inner<std::string, std::string_view, char>(s, delim, max_splits);
}
std::vector<std::wstring> split(std::wstring_view s, wchar_t delim, size_t max_splits) {
  return split_inner<std::wstring, std::wstring_view, char>(s, delim, max_splits);
}
std::vector<std::string_view> split_view(std::string_view s, char delim, size_t max_splits) {
  return split_inner<std::string_view, std::string_view, char>(s, delim, max_splits);
}
std::vector<std::wstring_view> split_view(std::wstring_view s, wchar_t delim, size_t max_splits) {
  return split_inner<std::wstring_view, std::wstring_view, char>(s, delim, max_splits);
}

template <typename RetT, typename InT, typename CharT>
std::vector<RetT> split_context_inner(InT s, CharT delim, size_t max_splits) {
  std::vector<RetT> ret;
  std::vector<CharT> paren_stack;
  bool char_is_escaped = false;

  size_t z, last_start = 0;
  for (z = 0; z < s.size(); z++) {
    if (!char_is_escaped && !paren_stack.empty() && (s[z] == paren_stack.back())) {
      paren_stack.pop_back();
      continue;
    }
    bool in_quoted_string = (!paren_stack.empty() && ((paren_stack.back() == '\'') || (paren_stack.back() == '\"')));
    if (char_is_escaped) {
      char_is_escaped = false;
    } else if (in_quoted_string && s[z] == '\\') {
      char_is_escaped = true;
    }
    if (!in_quoted_string) {
      if (s[z] == '(') {
        paren_stack.push_back(')');
      } else if (s[z] == '[') {
        paren_stack.push_back(']');
      } else if (s[z] == '{') {
        paren_stack.push_back('}');
      } else if (s[z] == '<') {
        paren_stack.push_back('>');
      } else if (s[z] == '\'') {
        paren_stack.push_back('\'');
      } else if (s[z] == '\"') {
        paren_stack.push_back('\"');
      } else if (paren_stack.empty() && (s[z] == delim) && (!max_splits || (ret.size() < max_splits))) {
        ret.emplace_back(s.substr(last_start, z - last_start));
        last_start = z + 1;
      }
    }
  }

  if (z >= last_start) {
    ret.emplace_back(s.substr(last_start));
  }

  if (paren_stack.size()) {
    throw std::runtime_error("unbalanced parentheses in split_context");
  }

  return ret;
}

std::vector<std::string> split_context(std::string_view s, char delim, size_t max_splits) {
  return split_context_inner<std::string, std::string_view, char>(s, delim, max_splits);
}
std::vector<std::string_view> split_context_view(std::string_view s, char delim, size_t max_splits) {
  return split_context_inner<std::string_view, std::string_view, char>(s, delim, max_splits);
}

std::vector<std::string> split_args(std::string_view s) {
  std::vector<std::string> ret;
  char current_quote = 0;
  bool in_space_between_args = true;

  for (size_t z = 0; z < s.size(); z++) {
    bool can_be_space = true;
    char to_write = 0;
    if (current_quote) {
      can_be_space = false;
      if (s[z] == current_quote) {
        current_quote = 0;
      } else if (s[z] == '\\') {
        z++;
        if (z >= s.size()) {
          throw std::runtime_error("incomplete escape sequence");
        }
        to_write = s[z];
      } else {
        to_write = s[z];
      }
    } else if ((s[z] == '\"') || (s[z] == '\'')) {
      current_quote = s[z];
    } else if (s[z] == '\\') {
      can_be_space = false;
      z++;
      if (z >= s.size()) {
        throw std::runtime_error("incomplete escape sequence");
      }
      to_write = s[z];
    } else {
      to_write = s[z];
    }

    if (to_write) {
      bool is_space_between_args = can_be_space && isblank(to_write);
      if (is_space_between_args && in_space_between_args) {
        // Nothing
      } else if (!is_space_between_args && in_space_between_args) {
        // Start of another arg
        ret.emplace_back();
        if (to_write) {
          ret.back().push_back(to_write);
        }
        in_space_between_args = false;
      } else if (is_space_between_args && !in_space_between_args) {
        in_space_between_args = true;
      } else { // !is_space_between_args && !in_space_between_args
        ret.back().push_back(to_write);
      }
    }
  }

  if (current_quote) {
    throw std::runtime_error("unterminated quoted string");
  }

  return ret;
}

size_t skip_whitespace(std::string_view s, size_t offset) {
  while (offset < s.length() && (s[offset] == ' ' || s[offset] == '\t' || s[offset] == '\r' || s[offset] == '\n')) {
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

size_t skip_non_whitespace(std::string_view s, size_t offset) {
  while (offset < s.length() && (s[offset] != ' ' && s[offset] != '\t' && s[offset] != '\r' && s[offset] != '\n')) {
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

size_t skip_word(std::string_view s, size_t offset) {
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
  return std::format("{} ({})", error, buffer);
}

std::string vformat_color_escape(TerminalFormat color, va_list va) {
  std::string fmt("\033");

  do {
    fmt += std::format("{:c}{}", (fmt[fmt.size() - 1] == '\033') ? '[' : ';', static_cast<ssize_t>(color));
    color = va_arg(va, TerminalFormat);
  } while (color != TerminalFormat::END);

  fmt += 'm';
  return fmt;
}

std::string format_color_escape(TerminalFormat color, ...) {
  va_list va;
  va_start(va, color);
  std::string ret = vformat_color_escape(color, va);
  va_end(va);
  return ret;
}

void print_color_escape(FILE* stream, TerminalFormat color, ...) {
  va_list va;
  va_start(va, color);
  std::string fmt = vformat_color_escape(color, va);
  va_end(va);
  fwrite(fmt.data(), fmt.size(), 1, stream);
}

void print_indent(FILE* stream, int indent_level) {
  for (; indent_level > 0; indent_level--) {
    fputc(' ', stream);
    fputc(' ', stream);
  }
}

bool print_binary_diff(
    FILE* stream,
    const void* data1v,
    size_t size1,
    const void* data2v,
    size_t size2,
    bool use_color,
    size_t context_lines,
    uint64_t base_offset) {
  const uint8_t* data1 = reinterpret_cast<const uint8_t*>(data1v);
  const uint8_t* data2 = reinterpret_cast<const uint8_t*>(data2v);

  size_t max_data_size = std::max<size_t>(size1, size2);
  int offset_width_digits;
  if (base_offset + max_data_size > 0x100000000) {
    offset_width_digits = 16;
  } else if (base_offset + max_data_size > 0x10000) {
    offset_width_digits = 8;
  } else if (base_offset + max_data_size > 0x100) {
    offset_width_digits = 4;
  } else {
    offset_width_digits = 2;
  }

  bool is_identical = true;
  auto print_diff_line = [&](char left_ch, const uint8_t* data, size_t size, size_t line_index, uint16_t diff_flags, TerminalFormat color) {
    size_t line_start_offset = line_index * 0x10;
    if (use_color) {
      print_color_escape(stream, color, TerminalFormat::END);
    }
    uint64_t address = base_offset + line_start_offset;
    fwrite_fmt(stream, "{:c} {:0>{}X} |", left_ch, address, offset_width_digits);
    for (size_t within_line_offset = 0; within_line_offset < 0x10; within_line_offset++) {
      size_t offset = (line_index * 0x10) + within_line_offset;
      if (offset < size) {
        if (use_color && (diff_flags & (1 << within_line_offset))) {
          print_color_escape(stream, color, TerminalFormat::BOLD, TerminalFormat::END);
          fwrite_fmt(stream, " {:02X}", data[offset]);
          print_color_escape(stream, TerminalFormat::NORMAL, color, TerminalFormat::END);
        } else {
          fwrite_fmt(stream, " {:02X}", data[offset]);
        }
      } else {
        fwrite_fmt(stream, "   ");
      }
    }
    fwrite_fmt(stream, " | ");
    for (size_t within_line_offset = 0; within_line_offset < 0x10; within_line_offset++) {
      size_t offset = (line_index * 0x10) + within_line_offset;
      if (offset < size) {
        char ch = data[offset];
        if (ch < 0x20 || ch > 0x7E) {
          ch = ' ';
        }
        if (use_color && (diff_flags & (1 << within_line_offset))) {
          print_color_escape(stream, color, TerminalFormat::BOLD, TerminalFormat::END);
          fputc(ch, stream);
          print_color_escape(stream, TerminalFormat::NORMAL, color, TerminalFormat::END);
        } else {
          fputc(ch, stream);
        }
      } else {
        fputc(' ', stream);
      }
    }
    if (use_color) {
      print_color_escape(stream, TerminalFormat::NORMAL, TerminalFormat::END);
    }
    fputc('\n', stream);
  };

  auto print_diff_line_pair = [&](size_t line_index, uint16_t diff_flags) -> void {
    size_t line_start_offset = line_index * 0x10;
    if (diff_flags == 0) {
      print_diff_line(' ', data1, size1, line_index, diff_flags, TerminalFormat::NORMAL);
    } else {
      if (line_start_offset < size1) {
        print_diff_line('-', data1, size1, line_index, diff_flags, TerminalFormat::FG_RED);
      }
      if (line_start_offset < size2) {
        print_diff_line('+', data2, size2, line_index, diff_flags, TerminalFormat::FG_GREEN);
      }
    }
  };

  size_t first_unprinted_line_index = 0;
  ssize_t last_different_line_index = -(context_lines + 1);
  size_t num_lines = ((max_data_size + 0x0F) >> 4);
  for (size_t line_index = 0; line_index < num_lines; line_index++) {
    uint16_t diff_flags = 0;
    for (size_t within_line_offset = 0; within_line_offset < 0x10; within_line_offset++) {
      size_t offset = (line_index * 0x10) + within_line_offset;
      uint16_t data1_value = (offset < size1) ? static_cast<uint8_t>(data1[offset]) : 0xFFFF;
      uint16_t data2_value = (offset < size2) ? static_cast<uint8_t>(data2[offset]) : 0xFFFF;
      if (data1_value != data2_value) {
        is_identical = false;
        diff_flags |= (1 << within_line_offset);
      }
    }
    if (diff_flags == 0) {
      if (static_cast<ssize_t>(line_index) <= last_different_line_index + static_cast<ssize_t>(context_lines)) {
        print_diff_line_pair(line_index, diff_flags);
        first_unprinted_line_index = line_index + 1;
      }

    } else {
      bool has_unprinted_gap;
      size_t chunk_start_line_index;
      if ((first_unprinted_line_index + context_lines) >= line_index) {
        chunk_start_line_index = first_unprinted_line_index;
        has_unprinted_gap = false;
      } else {
        chunk_start_line_index = line_index - context_lines;
        has_unprinted_gap = true;
      }

      if (has_unprinted_gap) {
        fwrite_fmt(stream, "  ...\n");
      }

      for (size_t z = chunk_start_line_index; z < line_index; z++) {
        print_diff_line_pair(z, 0x0000);
      }
      print_diff_line_pair(line_index, diff_flags);
      first_unprinted_line_index = line_index + 1;
      last_different_line_index = line_index;
    }
  }

  if ((first_unprinted_line_index < num_lines) && !is_identical) {
    fwrite_fmt(stream, "  ...\n");
  }
  return is_identical;
}

static inline void add_mask_bits(std::string* mask, bool mask_enabled, size_t num_bytes) {
  if (!mask) {
    return;
  }
  mask->append(num_bytes, mask_enabled ? '\xFF' : '\x00');
}

std::string parse_data_string(const std::string& s, std::string* mask, uint64_t flags) {
  bool allow_files = flags & ParseDataFlags::ALLOW_FILES;

  const char* in = s.c_str();

  std::string data;
  if (mask) {
    mask->clear();
  }

#ifdef PHOSG_BIG_ENDIAN
  constexpr bool host_big_endian = true;
#else
  constexpr bool host_big_endian = false;
#endif

  uint8_t chr = 0;
  bool reading_string = false;
  bool reading_unicode_string = false;
  bool reading_comment = false;
  bool reading_multiline_comment = false;
  bool reading_high_nybble = true;
  bool reading_filename = false;
  bool big_endian = false;
  bool mask_enabled = true;
  std::string filename;
  while (in[0]) {
    bool read_nybble = 0;

    if (reading_comment) {
      // If between // and a newline, don't write to output buffer
      if (in[0] == '\n') {
        reading_comment = false;
      }
      in++;

    } else if (reading_multiline_comment) {
      // If between /* and */, don't write to output buffer
      if ((in[0] == '*') && (in[1] == '/')) {
        reading_multiline_comment = 0;
        in += 2;
      } else {
        in++;
      }

    } else if (reading_string) {
      // If between quotes, read bytes to output buffer, unescaping where needed
      if (in[0] == '\"') {
        reading_string = 0;
        in++;

      } else if (in[0] == '\\') {
        if (!in[1]) {
          return data;
        } else if (in[1] == 'n') {
          data += '\n';
        } else if (in[1] == 'r') {
          data += '\r';
        } else if (in[1] == 't') {
          data += '\t';
        } else if (in[1] == '\"') {
          data += '\"';
        } else if (in[1] == '\'') {
          data += '\'';
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

    } else if (reading_unicode_string) {
      // If between single quotes, word-expand bytes to output buffer, unescaping
      if (in[0] == '\'') {
        reading_unicode_string = 0;
        in++;

      } else if (in[0] == '\\') {
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
        if (big_endian != host_big_endian) {
          value = bswap16(value);
        }
        data.append((const char*)&value, 2);
        add_mask_bits(mask, mask_enabled, 2);
        in += 2;

      } else {
        int16_t value = in[0];
        if (big_endian != host_big_endian) {
          value = bswap16(value);
        }
        data.append((const char*)&value, 2);
        add_mask_bits(mask, mask_enabled, 2);
        in++;
      }

    } else if (reading_filename) {
      // If between <>, read a file name, then insert the file contents into the buffer
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

    } else if (in[0] == '?') {
      // ? inverts mask_enabled
      mask_enabled = !mask_enabled;
      in++;

    } else if (in[0] == '$') {
      // $ changes the endianness
      big_endian = !big_endian;
      in++;

    } else if (in[0] == '#') { // 8-bit
      // # signifies a decimal number
      in++;
      if (in[0] == '#') { // 16-bit
        in++;
        if (in[0] == '#') { // 32-bit
          in++;
          if (in[0] == '#') { // 64-bit
            in++;
            uint64_t value = strtoull(in, const_cast<char**>(&in), 0);
            if (big_endian != host_big_endian) {
              value = bswap64(value);
            }
            data.append((const char*)&value, 8);
            add_mask_bits(mask, mask_enabled, 8);

          } else {
            uint32_t value = strtoull(in, const_cast<char**>(&in), 0);
            if (big_endian != host_big_endian) {
              value = bswap32(value);
            }
            data.append((const char*)&value, 4);
            add_mask_bits(mask, mask_enabled, 4);
          }

        } else {
          uint16_t value = strtoull(in, const_cast<char**>(&in), 0);
          if (big_endian != host_big_endian) {
            value = bswap16(value);
          }
          data.append((const char*)&value, 2);
          add_mask_bits(mask, mask_enabled, 2);
        }

      } else {
        data.append(1, (char)strtoull(in, const_cast<char**>(&in), 0));
        add_mask_bits(mask, mask_enabled, 1);
      }

    } else if (in[0] == '%') {
      // % is a float, %% is a double
      in++;
      if (in[0] == '%') {
        in++;

        uint64_t value;
        *(double*)&value = strtod(in, const_cast<char**>(&in));
        if (big_endian != host_big_endian) {
          value = bswap64(value);
        }
        data.append((const char*)&value, 8);
        add_mask_bits(mask, mask_enabled, 8);

      } else {
        uint32_t value;
        *(float*)&value = strtof(in, const_cast<char**>(&in));
        if (big_endian != host_big_endian) {
          value = bswap32(value);
        }
        data.append((const char*)&value, 4);
        add_mask_bits(mask, mask_enabled, 4);
      }

    } else {
      // Anything else is a hex digit
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

      } else if (in[0] == '<' && allow_files) {
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

std::string format_data_string(std::string_view data, std::string_view mask, uint64_t flags) {
  if (mask.size() != data.size()) {
    throw std::logic_error("data and mask sizes do not match");
  }
  return format_data_string(data.data(), data.size(), mask.data(), flags);
}

std::string format_data_string(std::string_view data, uint64_t flags) {
  return format_data_string(data.data(), data.size(), nullptr, flags);
}

std::string format_data_string(const void* vdata, size_t size, const void* vmask, uint64_t flags) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);
  const uint8_t* mask = reinterpret_cast<const uint8_t*>(vmask);

  // If all bytes are ASCII-printable, render as a string instead
  bool is_printable = !(flags & FormatDataStringFlags::SKIP_STRINGS);
  if (is_printable) {
    for (size_t z = 0; z < size && is_printable; z++) {
      if (data[z] != '\r' && data[z] != '\n' && data[z] != '\t' && (data[z] < 0x20 || data[z] > 0x7E)) {
        is_printable = false;
      }
    }
  }

  std::string ret;
  bool mask_enabled = true;
  if (is_printable) {
    ret += '\"';
    for (size_t x = 0; x < size; x++) {
      if (mask && ((bool)mask[x] != mask_enabled)) {
        mask_enabled = !mask_enabled;
        ret += "\"?\"";
      }
      if (data[x] == '\r') {
        ret += "\\r";
      } else if (data[x] == '\t') {
        ret += "\\t";
      } else if (data[x] == '\n') {
        ret += "\\n";
      } else if (data[x] == '\"') {
        ret += "\\\"";
      } else if (data[x] == '\'') {
        ret += "\\\'";
      } else {
        ret.push_back(data[x]);
      }
    }
    ret += '\"';
  } else {
    for (size_t x = 0; x < size; x++) {
      if (mask && ((bool)mask[x] != mask_enabled)) {
        mask_enabled = !mask_enabled;
        ret += '?';
      }
      ret += std::format("{:02X}", data[x]);
    }
  }
  return ret;
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

#if (SIZE_T_BITS == 8)

std::string format_size(size_t size, bool include_bytes) {
  return std::format("{} bytes", size);
}

#elif (SIZE_T_BITS == 16)

std::string format_size(size_t size, bool include_bytes) {
  if (size < KB_SIZE) {
    return std::format("{} bytes", size);
  }
  if (include_bytes) {
    return std::format("{} bytes ({:.02f} KB)", size, (float)size / KB_SIZE);
  } else {
    return std::format("{:.02f} KB", (float)size / KB_SIZE);
  }
}

#elif (SIZE_T_BITS == 32)

std::string format_size(size_t size, bool include_bytes) {
  if (size < KB_SIZE) {
    return std::format("{} bytes", size);
  }
  if (include_bytes) {
    if (size < MB_SIZE) {
      return std::format("{} bytes ({:.02f} KB)", size, (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return std::format("{} bytes ({:.02f} MB)", size, (float)size / MB_SIZE);
    }
    return std::format("{} bytes ({:.02f} GB)", size, (float)size / GB_SIZE);
  } else {
    if (size < MB_SIZE) {
      return std::format("{:.02f} KB", (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return std::format("{:.02f} MB", (float)size / MB_SIZE);
    }
    return std::format("{:.02f} GB", (float)size / GB_SIZE);
  }
}

#elif (SIZE_T_BITS == 64)

std::string format_size(size_t size, bool include_bytes) {
  if (size < KB_SIZE) {
    return std::format("{} bytes", size);
  }
  if (include_bytes) {
    if (size < MB_SIZE) {
      return std::format("{} bytes ({:.02f} KB)", size, (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return std::format("{} bytes ({:.02f} MB)", size, (float)size / MB_SIZE);
    }
    if (size < TB_SIZE) {
      return std::format("{} bytes ({:.02f} GB)", size, (float)size / GB_SIZE);
    }
    if (size < PB_SIZE) {
      return std::format("{} bytes ({:.02f} TB)", size, (float)size / TB_SIZE);
    }
    if (size < EB_SIZE) {
      return std::format("{} bytes ({:.02f} PB)", size, (float)size / PB_SIZE);
    }
    return std::format("{} bytes ({:.02f} EB)", size, (float)size / EB_SIZE);
  } else {
    if (size < MB_SIZE) {
      return std::format("{:.02f} KB", (float)size / KB_SIZE);
    }
    if (size < GB_SIZE) {
      return std::format("{:.02f} MB", (float)size / MB_SIZE);
    }
    if (size < TB_SIZE) {
      return std::format("{:.02f} GB", (float)size / GB_SIZE);
    }
    if (size < PB_SIZE) {
      return std::format("{:.02f} TB", (float)size / TB_SIZE);
    }
    if (size < EB_SIZE) {
      return std::format("{:.02f} PB", (float)size / PB_SIZE);
    }
    return std::format("{:.02f} EB", (float)size / EB_SIZE);
  }
}

#endif

size_t parse_size(const char* str) {
  // Input is like [0-9](\.[0-9]+)? *[KkMmGgTtPpEe]?[Bb]? - fortunately this can just be parsed left-to-right
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
  for (; *str == ' '; str++) {
  }
#if SIZE_T_BITS >= 16
  if (*str == 'K' || *str == 'k') {
    unit_scale = KB_SIZE;
#if SIZE_T_BITS >= 32
  } else if (*str == 'M' || *str == 'm') {
    unit_scale = MB_SIZE;
  } else if (*str == 'G' || *str == 'g') {
    unit_scale = GB_SIZE;
#if SIZE_T_BITS == 64
  } else if (*str == 'T' || *str == 't') {
    unit_scale = TB_SIZE;
  } else if (*str == 'P' || *str == 'p') {
    unit_scale = PB_SIZE;
  } else if (*str == 'E' || *str == 'e') {
    unit_scale = EB_SIZE;
#endif
#endif
  }
#endif

  return integer_part * unit_scale + static_cast<size_t>(fractional_part * unit_scale);
}

BitReader::BitReader() : owned_data(nullptr), data(nullptr), length(0), offset(0) {}

BitReader::BitReader(std::shared_ptr<std::string> data, size_t offset)
    : owned_data(data),
      data(reinterpret_cast<const uint8_t*>(data->data())),
      length(data->size() * 8),
      offset(offset) {}

BitReader::BitReader(const void* data, size_t size, size_t offset)
    : data(reinterpret_cast<const uint8_t*>(data)), length(size), offset(offset) {}

BitReader::BitReader(std::string_view data, size_t offset) : BitReader(data.data(), data.size() * 8, offset) {}

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
    throw std::invalid_argument("BitReader contents cannot be extended");
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
    throw std::logic_error("BitReader cannot return more than 64 bits at once");
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

BitWriter::BitWriter() : last_byte_unset_bits(0) {}

size_t BitWriter::size() const {
  return this->data.size() * 8 - this->last_byte_unset_bits;
}

void BitWriter::reset() {
  this->data.clear();
  this->last_byte_unset_bits = 0;
}

void BitWriter::truncate(size_t size) {
  if (size > ((this->data.size() * 8) - this->last_byte_unset_bits)) {
    throw std::logic_error("cannot extend a BitWriter via truncate()");
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

IOVecByteReader::IOVecByteReader(const struct iovec* iovs, size_t num_iovs) : iovs(iovs), num_iovs(num_iovs) {}

uint8_t IOVecByteReader::get_u8() {
  uint8_t ret = reinterpret_cast<const uint8_t*>(this->iovs[this->current_iov].iov_base)[this->offset_within_iov];
  this->offset_within_iov++;
  while ((this->current_iov < this->num_iovs) &&
      (this->offset_within_iov >= this->iovs[this->current_iov].iov_len)) {
    this->current_iov++;
    this->offset_within_iov = 0;
  }
  return ret;
}

size_t IOVecByteReader::size() const {
  size_t ret = 0;
  for (size_t z = 0; z < this->num_iovs; z++) {
    ret += this->iovs[z].iov_len;
  }
  return ret;
}

StringReader::StringReader() : owned_data(nullptr), data(nullptr), length(0), offset(0) {}

StringReader::StringReader(std::shared_ptr<std::string> data, size_t offset)
    : owned_data(data), data(reinterpret_cast<const uint8_t*>(data->data())), length(data->size()), offset(offset) {}

StringReader::StringReader(const void* data, size_t size, size_t offset)
    : data(reinterpret_cast<const uint8_t*>(data)), length(size), offset(offset) {}

StringReader::StringReader(std::string_view data, size_t offset) : StringReader(data.data(), data.size(), offset) {}

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
    throw std::invalid_argument("StringReader contents cannot be extended");
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
    throw std::out_of_range("skip beyond end of string");
  }
}

bool StringReader::skip_if(const void* data, size_t size) {
  if ((this->remaining() < size) || memcmp(this->peek(size), data, size)) {
    return false;
  } else {
    this->skip(size);
    return true;
  }
}

bool StringReader::eof() const {
  return (this->offset >= this->length);
}

std::string_view StringReader::all() const {
  return std::string_view(reinterpret_cast<const char*>(this->data), this->length);
}

StringReader StringReader::sub(size_t offset) const {
  return (offset > this->length)
      ? StringReader()
      : StringReader(reinterpret_cast<const char*>(this->data) + offset, this->length - offset);
}

StringReader StringReader::sub(size_t offset, size_t size) const {
  if (offset > this->length) {
    size = 0;
  } else if (offset + size > this->length) {
    size = std::max<ssize_t>(this->length - offset, 0);
  }
  return StringReader(reinterpret_cast<const char*>(this->data) + offset, size);
}

StringReader StringReader::subx(size_t offset) const {
  if (offset > this->length) {
    throw std::out_of_range("sub-reader begins beyond end of data");
  }
  return StringReader(reinterpret_cast<const char*>(this->data) + offset, this->length - offset);
}

StringReader StringReader::subx(size_t offset, size_t size) const {
  if (offset + size > this->length) {
    throw std::out_of_range("sub-reader begins or extends beyond end of data");
  }
  return StringReader(reinterpret_cast<const char*>(this->data) + offset, size);
}

StringReader StringReader::extract() {
  return this->extract(this->remaining());
}

StringReader StringReader::extract(size_t size) {
  auto ret = this->sub(this->offset, size);
  this->skip(ret.size());
  return ret;
}

StringReader StringReader::extractx(size_t size) {
  auto ret = this->subx(this->offset, size);
  this->skip(ret.size());
  return ret;
}

BitReader StringReader::sub_bits(size_t offset) const {
  return (offset > this->length)
      ? BitReader()
      : BitReader(reinterpret_cast<const char*>(this->data) + offset, (this->length - offset) * 8);
}

BitReader StringReader::sub_bits(size_t offset, size_t size) const {
  if (offset > this->length) {
    size = 0;
  } else if (offset + size > this->length) {
    size = std::max<ssize_t>(this->length - offset, 0);
  }
  return BitReader(reinterpret_cast<const char*>(this->data) + offset, size * 8);
}

BitReader StringReader::subx_bits(size_t offset) const {
  if (offset > this->length) {
    throw std::out_of_range("sub-reader begins beyond end of data");
  }
  return BitReader(reinterpret_cast<const char*>(this->data) + offset, (this->length - offset) * 8);
}

BitReader StringReader::subx_bits(size_t offset, size_t size) const {
  if ((offset > this->length) || (offset + size > this->length)) {
    throw std::out_of_range("sub-reader begins or extends beyond end of data");
  }
  return BitReader(reinterpret_cast<const char*>(this->data) + offset, size * 8);
}

BitReader StringReader::extract_bits() {
  return this->extract_bits(this->remaining());
}

BitReader StringReader::extract_bits(size_t size) {
  auto ret = this->sub_bits(this->offset, size);
  this->skip(ret.size());
  return ret;
}

BitReader StringReader::extractx_bits(size_t size) {
  auto ret = this->subx_bits(this->offset, size);
  this->skip(ret.size());
  return ret;
}

const char* StringReader::peek(size_t size) {
  if (this->offset + size <= this->length) {
    return reinterpret_cast<const char*>(this->data + this->offset);
  }
  throw std::out_of_range("not enough data to read");
}

std::string_view StringReader::read(size_t size, bool advance) {
  std::string_view ret = this->pread(this->offset, size);
  if (ret.size() && advance) {
    this->offset += ret.size();
  }
  return ret;
}

std::string_view StringReader::readx(size_t size, bool advance) {
  std::string_view ret = this->preadx(this->offset, size);
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

std::string_view StringReader::pread(size_t offset, size_t size) const {
  if (offset >= this->length) {
    return std::string_view{};
  }
  if (offset + size > this->length) {
    return std::string_view{reinterpret_cast<const char*>(this->data + offset), this->length - offset};
  }
  return std::string_view{reinterpret_cast<const char*>(this->data + offset), size};
}

std::string_view StringReader::preadx(size_t offset, size_t size) const {
  if (offset + size > this->length) {
    throw std::out_of_range("not enough data to read");
  }
  return std::string_view{reinterpret_cast<const char*>(this->data + offset), size};
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
    throw std::out_of_range("not enough data to read");
  }
  memcpy(data, this->data + offset, size);
}

std::string_view StringReader::get_line(bool advance) {
  if (this->eof()) {
    throw std::out_of_range("end of string");
  }

  size_t start_offset = this->offset;
  size_t ret_size = 0;
  char last_ch = '\0';
  for (;;) {
    size_t ch_offset = start_offset + ret_size;
    if (ch_offset >= this->length) {
      break;
    }
    uint8_t ch = this->pget_s8(ch_offset);
    if (ch != '\n') {
      last_ch = ch;
      ret_size++;
    } else {
      break;
    }
  }
  if (advance) {
    this->offset += (ret_size + 1);
  }
  if (last_ch == '\r') {
    ret_size--;
  }
  return std::string_view{reinterpret_cast<const char*>(this->data) + start_offset, ret_size};
}

std::string_view StringReader::get_cstr(bool advance) {
  std::string_view ret = this->pget_cstr(this->offset);
  if (advance) {
    this->offset += (ret.size() + 1);
  }
  return ret;
}

std::string_view StringReader::pget_cstr(size_t offset) const {
  size_t ret_size = 0;
  for (;;) {
    if (this->pget_s8(offset + ret_size) != 0) {
      ret_size++;
    } else {
      break;
    }
  }
  return std::string_view(reinterpret_cast<const char*>(this->data) + offset, ret_size);
}

void StringWriter::reset() {
  this->contents.clear();
}

void StringWriter::write(const void* data, size_t size) {
  this->contents.append(reinterpret_cast<const char*>(data), size);
}

void StringWriter::write(std::string_view data) {
  this->contents.append(data);
}

size_t count_zeroes(const void* vdata, size_t size, size_t stride) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);
  size_t zero_count = 0;
  for (size_t z = 0; z < size; z += stride) {
    if (data[z] == 0) {
      zero_count++;
    }
  }
  return zero_count;
}

void BlockStringWriter::write(const void* data, size_t size) {
  this->blocks.emplace_back(reinterpret_cast<const char*>(data), size);
}

void BlockStringWriter::write(std::string_view data) {
  this->blocks.emplace_back(data);
}

void BlockStringWriter::add(std::string&& data) {
  this->blocks.emplace_back(std::move(data));
}

std::string BlockStringWriter::close(const char* separator) {
  return join(this->blocks, separator);
}

} // namespace phosg
