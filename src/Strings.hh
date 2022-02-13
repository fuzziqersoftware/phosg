#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Platform.hh"


std::unique_ptr<void, void (*)(void*)> malloc_unique(size_t size);

bool starts_with(const std::string& s, const std::string& start);
bool ends_with(const std::string& s, const std::string& end);

void strip_trailing_zeroes(std::string& s);
void strip_trailing_whitespace(std::string& s);

std::string escape_quotes(const std::string& s);
std::string escape_url(const std::string& s, bool escape_slash = false);

std::string string_printf(const char* fmt, ...)
__attribute__((format(printf, 1, 2)));
std::wstring wstring_printf(const wchar_t* fmt, ...);
std::string string_vprintf(const char* fmt, va_list va);
std::wstring wstring_vprintf(const wchar_t* fmt, va_list va);

uint8_t value_for_hex_char(char x);

#define DEBUG 0
#define INFO 1
#define WARNING 2
#ifndef PHOSG_WINDOWS
// wingdi.h defined ERROR as 0, lolz
#define ERROR 3
#endif

int log_level();
void set_log_level(int new_level);
void log(int level, const char* fmt, ...)
__attribute__((format(printf, 2, 3)));

std::vector<std::string> split(const std::string& s, char delim);
std::vector<std::wstring> split(const std::wstring& s, wchar_t delim);
std::vector<std::string> split_context(const std::string& s, char delim);

template<typename ItemContainerT, typename DelimiterT>
std::string join(const ItemContainerT& items, DelimiterT& delim) {
  std::string ret;
  for (const auto& item : items) {
    if (!ret.empty()) {
      ret += delim;
    }
    ret += item;
  }
  return ret;
}

size_t skip_whitespace(const std::string& s, size_t offset);
size_t skip_whitespace(const char* s, size_t offset);
size_t skip_non_whitespace(const std::string& s, size_t offset);
size_t skip_non_whitespace(const char* s, size_t offset);
size_t skip_word(const std::string& s, size_t offset);
size_t skip_word(const char* s, size_t offset);

std::string string_for_error(int error);

enum class TerminalFormat {
  END        = -1,
  NORMAL     = 0,
  BOLD       = 1,
  UNDERLINE  = 4,
  BLINK      = 5,
  INVERSE    = 7,
  FG_BLACK   = 30,
  FG_RED     = 31,
  FG_GREEN   = 32,
  FG_YELLOW  = 33,
  FG_BLUE    = 34,
  FG_MAGENTA = 35,
  FG_CYAN    = 36,
  FG_GRAY    = 37,
  FG_WHITE   = 38,
  BG_BLACK   = 40,
  BG_RED     = 41,
  BG_GREEN   = 42,
  BG_YELLOW  = 43,
  BG_BLUE    = 44,
  BG_MAGENTA = 45,
  BG_CYAN    = 46,
  BG_GRAY    = 47,
  BG_WHITE   = 48,
};

std::string vformat_color_escape(TerminalFormat color, va_list va);
std::string format_color_escape(TerminalFormat color, ...);
void print_color_escape(FILE* stream, TerminalFormat color, ...);

void print_indent(FILE* stream, int indent_level);

enum PrintDataFlags {
  UseColor = 1, // use terminal escape codes to show differences
  PrintAscii = 2, // print ascii view on the right
  PrintFloat = 4, // print float view on the right
  PrintDouble = 8, // print double view on the right
  ReverseEndian = 16, // floats/doubles should be byteswapped
  CollapseZeroLines = 32, // skips lines of all zeroes
};

void print_data(FILE* stream, const void* _data, uint64_t size,
    uint64_t address = 0, const void* _prev = nullptr,
    uint64_t flags = PrintDataFlags::PrintAscii);
void print_data(FILE* stream, const std::string& data, uint64_t address = 0,
    const void* prev = nullptr, uint64_t flags = PrintDataFlags::PrintAscii);

std::string parse_data_string(const std::string& s, std::string* mask = nullptr);
std::string format_data_string(const std::string& data, const std::string* mask = nullptr);
std::string format_data_string(const void* data, size_t size, const void* mask = nullptr);

std::string format_time(struct timeval* tv = nullptr);
std::string format_duration(uint64_t usecs, int8_t subsecond_precision = -1);
std::string format_size(size_t size, bool include_bytes = false);
size_t parse_size(const char* str);

class BitReader {
public:
  BitReader();
  explicit BitReader(std::shared_ptr<std::string> data, size_t offset = 0);
  BitReader(const void* data, size_t size, size_t offset = 0);
  BitReader(const std::string& data, size_t offset = 0);
  virtual ~BitReader() = default;

  size_t where() const;
  size_t size() const;
  size_t remaining() const;
  void truncate(size_t new_size);
  void go(size_t offset);
  void skip(size_t bits);
  bool eof() const;

  uint64_t pread(size_t offset, uint8_t size = 1);
  uint64_t read(uint8_t size = 1, bool advance = true);

private:
  std::shared_ptr<std::string> owned_data;
  const uint8_t* data;
  size_t length;
  size_t offset;
};

class StringReader {
public:
  StringReader();
  explicit StringReader(std::shared_ptr<std::string> data, size_t offset = 0);
  StringReader(const void* data, size_t size, size_t offset = 0);
  StringReader(const std::string& data, size_t offset = 0);
  virtual ~StringReader() = default;

  size_t where() const;
  size_t size() const;
  size_t remaining() const;
  void truncate(size_t new_size);
  void go(size_t offset);
  void skip(size_t bytes);
  bool eof() const;
  std::string all() const;

  StringReader sub(size_t offset, size_t size) const;

  const char* peek(size_t size);

  std::string read(size_t size, bool advance = true);
  std::string readx(size_t size, bool advance = true);
  size_t read_into(void* data, size_t size, bool advance = true);
  void readx_into(void* data, size_t size, bool advance = true);
  std::string pread(size_t offset, size_t size) const;
  std::string preadx(size_t offset, size_t size) const;
  size_t pread_into(size_t offset, void* data, size_t size) const;
  void preadx_into(size_t offset, void* data, size_t size) const;

  template <typename T> const T& get(bool advance = true) {
    if (this->offset > this->length - sizeof(T)) {
      throw std::out_of_range("end of string");
    }
    const T& ret = *reinterpret_cast<const T*>(this->data + this->offset);
    if (advance) {
      this->offset += sizeof(T);
    }
    return ret;
  }

  template <typename T> const T& pget(size_t offset) const {
    if (offset > this->length - sizeof(T)) {
      throw std::out_of_range("end of string");
    }
    return *reinterpret_cast<const T*>(this->data + offset);
  }

  template <typename T> T get_sw(bool advance = true) {
    T ret = this->get<T>(advance);
    ret.byteswap();
    return ret;
  }

  template <typename T> T pget_sw(size_t offset) const {
    T ret = this->pget<T>(offset);
    ret.byteswap();
    return ret;
  }

  inline char get_ch(bool advance = true) {
    return this->get_s8(advance);
  }

  uint8_t get_u8(bool advance = true);
  int8_t get_s8(bool advance = true);
  uint16_t get_u16(bool advance = true);
  int16_t get_s16(bool advance = true);
  uint32_t get_u24(bool advance = true);
  int32_t get_s24(bool advance = true);
  uint32_t get_u32(bool advance = true);
  int32_t get_s32(bool advance = true);
  uint64_t get_u48(bool advance = true);
  int64_t get_s48(bool advance = true);
  uint64_t get_u64(bool advance = true);
  int64_t get_s64(bool advance = true);
  float get_f32(bool advance = true);
  double get_f64(bool advance = true);

  uint16_t get_u16r(bool advance = true);
  int16_t get_s16r(bool advance = true);
  uint32_t get_u24r(bool advance = true);
  int32_t get_s24r(bool advance = true);
  uint32_t get_u32r(bool advance = true);
  int32_t get_s32r(bool advance = true);
  uint64_t get_u48r(bool advance = true);
  int64_t get_s48r(bool advance = true);
  uint64_t get_u64r(bool advance = true);
  int64_t get_s64r(bool advance = true);
  float get_f32r(bool advance = true);
  double get_f64r(bool advance = true);

  inline char pget_ch(size_t offset) const {
    return this->pget_s8(offset);
  }

  uint8_t pget_u8(size_t offset) const;
  int8_t pget_s8(size_t offset) const;
  uint16_t pget_u16(size_t offset) const;
  int16_t pget_s16(size_t offset) const;
  uint32_t pget_u24(size_t offset) const;
  int32_t pget_s24(size_t offset) const;
  uint32_t pget_u32(size_t offset) const;
  int32_t pget_s32(size_t offset) const;
  uint64_t pget_u48(size_t offset) const;
  int64_t pget_s48(size_t offset) const;
  uint64_t pget_u64(size_t offset) const;
  int64_t pget_s64(size_t offset) const;
  float pget_f32(size_t offset) const;
  double pget_f64(size_t offset) const;

  uint16_t pget_u16r(size_t offset) const;
  int16_t pget_s16r(size_t offset) const;
  uint32_t pget_u24r(size_t offset) const;
  int32_t pget_s24r(size_t offset) const;
  uint32_t pget_u32r(size_t offset) const;
  int32_t pget_s32r(size_t offset) const;
  uint64_t pget_u48r(size_t offset) const;
  int64_t pget_s48r(size_t offset) const;
  uint64_t pget_u64r(size_t offset) const;
  int64_t pget_s64r(size_t offset) const;
  float pget_f32r(size_t offset) const;
  double pget_f64r(size_t offset) const;

  std::string get_cstr(bool advance = true);
  std::string pget_cstr(size_t offset) const;

private:
  std::shared_ptr<std::string> owned_data;
  const uint8_t* data;
  size_t length;
  size_t offset;
};

class StringWriter {
public:
  StringWriter() = default;
  ~StringWriter() = default;

  size_t size() const;

  void write(const std::string& data);

  template <typename T> void put(const T& v) {
    this->data.append(reinterpret_cast<const char*>(&v), sizeof(v));
  }

  template <typename T> void put_sw(const T& v) {
    this->data.append(reinterpret_cast<const char*>(&v), sizeof(v));
    reinterpret_cast<T*>(this->data[this->data.size() - sizeof(v)])->byteswap();
  }

  void put_u8(uint8_t v);
  void put_s8(int8_t v);
  void put_u16(uint16_t v);
  void put_s16(int16_t v);
  void put_u24(uint32_t v);
  void put_s24(int32_t v);
  void put_u32(uint32_t v);
  void put_s32(int32_t v);
  void put_u48(uint64_t v);
  void put_s48(int64_t v);
  void put_u64(uint64_t v);
  void put_s64(int64_t v);

  void put_u16r(uint16_t v);
  void put_s16r(int16_t v);
  void put_u24r(uint32_t v);
  void put_s24r(int32_t v);
  void put_u32r(uint32_t v);
  void put_s32r(int32_t v);
  void put_u48r(uint64_t v);
  void put_s48r(int64_t v);
  void put_u64r(uint64_t v);
  void put_s64r(int64_t v);

  inline std::string& str() {
    return this->data;
  }

private:
  std::string data;
};

template <typename T>
class StringBuffer : std::string {
public:
  StringBuffer(size_t size = sizeof(T)) : std::string(size, '\0') { }
  virtual ~StringBuffer() = default;

  T* buffer() {
    return reinterpret_cast<T*>(this->data());
  }
};

template <typename T>
T* data_at(std::string& s, size_t offset = 0) {
  return reinterpret_cast<T*>(s.data() + offset);
}
