#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Platform.hh"
#include "Encoding.hh"


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

std::vector<std::string> split(const std::string& s, char delim, size_t max_splits = 0);
std::vector<std::wstring> split(const std::wstring& s, wchar_t delim, size_t max_splits = 0);
std::vector<std::string> split_context(const std::string& s, char delim, size_t max_splits = 0);

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
  UseColor          = 0x01, // use terminal escape codes to show differences
  PrintAscii        = 0x02, // print ascii view on the right
  PrintFloat        = 0x04, // print float view on the right
  PrintDouble       = 0x08, // print double view on the right
  ReverseEndian     = 0x10, // floats/doubles should be byteswapped
  CollapseZeroLines = 0x20, // skips lines of all zeroes
  SkipSeparator     = 0x40, // instead of " | ", print just " "
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

  StringReader sub(size_t offset) const;
  StringReader sub(size_t offset, size_t size) const;
  StringReader subx(size_t offset) const;
  StringReader subx(size_t offset, size_t size) const;

  const char* peek(size_t size);

  std::string read(size_t size, bool advance = true);
  std::string readx(size_t size, bool advance = true);
  size_t read(void* data, size_t size, bool advance = true);
  void readx(void* data, size_t size, bool advance = true);
  std::string pread(size_t offset, size_t size) const;
  std::string preadx(size_t offset, size_t size) const;
  size_t pread(size_t offset, void* data, size_t size) const;
  void preadx(size_t offset, void* data, size_t size) const;

  template <typename T> const T& pget(size_t offset, size_t size = sizeof(T)) const {
    if (offset > this->length - size) {
      throw std::out_of_range("end of string");
    }
    return *reinterpret_cast<const T*>(this->data + offset);
  }

  template <typename T> const T& get(bool advance = true, size_t size = sizeof(T)) {
    const T& ret = this->pget<T>(this->offset, size);
    if (advance) {
      this->offset += size;
    }
    return ret;
  }

  inline uint8_t get_u8(bool advance = true)     { return this->get<uint8_t>(advance); }
  inline int8_t get_s8(bool advance = true)      { return this->get<int8_t>(advance); }
  inline uint8_t pget_u8(size_t offset) const    { return this->pget<uint8_t>(offset); }
  inline int8_t pget_s8(size_t offset) const     { return this->pget<int8_t>(offset); }

  inline uint16_t get_u16b(bool advance = true)  { return this->get<be_uint16_t>(advance); }
  inline uint16_t get_u16l(bool advance = true)  { return this->get<le_uint16_t>(advance); }
  inline int16_t get_s16b(bool advance = true)   { return this->get<be_int16_t>(advance); }
  inline int16_t get_s16l(bool advance = true)   { return this->get<le_int16_t>(advance); }
  inline uint16_t pget_u16b(size_t offset) const { return this->pget<be_uint16_t>(offset); }
  inline uint16_t pget_u16l(size_t offset) const { return this->pget<le_uint16_t>(offset); }
  inline int16_t pget_s16b(size_t offset) const  { return this->pget<be_int16_t>(offset); }
  inline int16_t pget_s16l(size_t offset) const  { return this->pget<le_int16_t>(offset); }

  inline uint32_t get_u32b(bool advance = true)  { return this->get<be_uint32_t>(advance); }
  inline uint32_t get_u32l(bool advance = true)  { return this->get<le_uint32_t>(advance); }
  inline int32_t get_s32b(bool advance = true)   { return this->get<be_int32_t>(advance); }
  inline int32_t get_s32l(bool advance = true)   { return this->get<le_int32_t>(advance); }
  inline uint32_t pget_u32b(size_t offset) const { return this->pget<be_uint32_t>(offset); }
  inline uint32_t pget_u32l(size_t offset) const { return this->pget<le_uint32_t>(offset); }
  inline int32_t pget_s32b(size_t offset) const  { return this->pget<be_int32_t>(offset); }
  inline int32_t pget_s32l(size_t offset) const  { return this->pget<le_int32_t>(offset); }

  inline uint64_t get_u64b(bool advance = true)  { return this->get<be_uint64_t>(advance); }
  inline uint64_t get_u64l(bool advance = true)  { return this->get<le_uint64_t>(advance); }
  inline int64_t get_s64b(bool advance = true)   { return this->get<be_int64_t>(advance); }
  inline int64_t get_s64l(bool advance = true)   { return this->get<le_int64_t>(advance); }
  inline uint64_t pget_u64b(size_t offset) const { return this->pget<be_uint64_t>(offset); }
  inline uint64_t pget_u64l(size_t offset) const { return this->pget<le_uint64_t>(offset); }
  inline int64_t pget_s64b(size_t offset) const  { return this->pget<be_int64_t>(offset); }
  inline int64_t pget_s64l(size_t offset) const  { return this->pget<le_int64_t>(offset); }

  inline float get_f32b(bool advance = true)     { return this->get<be_float>(advance); }
  inline float get_f32l(bool advance = true)     { return this->get<le_float>(advance); }
  inline float pget_f32b(size_t offset) const    { return this->pget<be_float>(offset); }
  inline float pget_f32l(size_t offset) const    { return this->pget<le_float>(offset); }

  inline double get_f64b(bool advance = true)    { return this->get<be_double>(advance); }
  inline double get_f64l(bool advance = true)    { return this->get<le_double>(advance); }
  inline double pget_f64b(size_t offset) const   { return this->pget<be_double>(offset); }
  inline double pget_f64l(size_t offset) const   { return this->pget<le_double>(offset); }

  inline uint32_t get_u24b(bool advance = true) {
    uint32_t ret = this->pget_u24b(this->offset);
    if (advance) {
      this->offset += 3;
    }
    return ret;
  }
  inline uint32_t get_u24l(bool advance = true) {
    uint32_t ret = this->pget_u24l(this->offset);
    if (advance) {
      this->offset += 3;
    }
    return ret;
  }
  inline int32_t get_s24b(bool advance = true) { return ext24(this->get_u24b(advance)); }
  inline int32_t get_s24l(bool advance = true) { return ext24(this->get_u24l(advance)); }

  inline uint32_t pget_u24b(size_t offset) const {
    if (offset >= this->length - 2) {
      throw std::out_of_range("end of string");
    }
    return (this->data[offset] << 16) | (this->data[offset + 1] << 8) | this->data[offset + 2];
  }
  inline uint32_t pget_u24l(size_t offset) const {
    if (offset >= this->length - 2) {
      throw std::out_of_range("end of string");
    }
    return this->data[offset] | (this->data[offset + 1] << 8) | (this->data[offset + 2] << 16);
  }
  inline int32_t pget_s24b(size_t offset) const { return ext24(this->pget_u24b(offset)); }
  inline int32_t pget_s24l(size_t offset) const { return ext24(this->pget_u24l(offset)); }

  inline uint64_t get_u48b(bool advance = true) {
    uint64_t ret = this->pget_u48b(this->offset);
    if (advance) {
      this->offset += 6;
    }
    return ret;
  }
  inline uint64_t get_u48l(bool advance = true) {
    uint64_t ret = this->pget_u48l(this->offset);
    if (advance) {
      this->offset += 6;
    }
    return ret;
  }
  inline int64_t get_s48b(bool advance = true) { return ext48(this->get_u48b(advance)); }
  inline int64_t get_s48l(bool advance = true) { return ext48(this->get_u48l(advance)); }
  inline uint64_t pget_u48b(size_t offset) const {
    if (offset >= this->length - 5) {
      throw std::out_of_range("end of string");
    }
    return (static_cast<uint64_t>(this->data[offset]) << 40) |
           (static_cast<uint64_t>(this->data[offset + 1]) << 32) |
           (static_cast<uint64_t>(this->data[offset + 2]) << 24) |
           (static_cast<uint64_t>(this->data[offset + 3]) << 16) |
           (static_cast<uint64_t>(this->data[offset + 4]) << 8) |
           (static_cast<uint64_t>(this->data[offset + 5]));
  }
  inline uint64_t pget_u48l(size_t offset) const {
    if (offset >= this->length - 5) {
      throw std::out_of_range("end of string");
    }
    return (static_cast<uint64_t>(this->data[offset])) |
           (static_cast<uint64_t>(this->data[offset + 1]) << 8) |
           (static_cast<uint64_t>(this->data[offset + 2]) << 16) |
           (static_cast<uint64_t>(this->data[offset + 3]) << 24) |
           (static_cast<uint64_t>(this->data[offset + 4]) << 32) |
           (static_cast<uint64_t>(this->data[offset + 5]) << 40);
  }
  inline int64_t pget_s48b(size_t offset) const { return ext48(this->pget_u48b(offset)); }
  inline int64_t pget_s48l(size_t offset) const { return ext48(this->pget_u48l(offset)); }

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

  void write(const void* data, size_t size);
  void write(const std::string& data);

  template <typename T> void put(const T& v) {
    this->data.append(reinterpret_cast<const char*>(&v), sizeof(v));
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

  void pput_u8(size_t offset, uint8_t v);
  void pput_s8(size_t offset, int8_t v);
  void pput_u16(size_t offset, uint16_t v);
  void pput_s16(size_t offset, int16_t v);
  void pput_u24(size_t offset, uint32_t v);
  void pput_s24(size_t offset, int32_t v);
  void pput_u32(size_t offset, uint32_t v);
  void pput_s32(size_t offset, int32_t v);
  void pput_u48(size_t offset, uint64_t v);
  void pput_s48(size_t offset, int64_t v);
  void pput_u64(size_t offset, uint64_t v);
  void pput_s64(size_t offset, int64_t v);

  void pput_u16r(size_t offset, uint16_t v);
  void pput_s16r(size_t offset, int16_t v);
  void pput_u24r(size_t offset, uint32_t v);
  void pput_s24r(size_t offset, int32_t v);
  void pput_u32r(size_t offset, uint32_t v);
  void pput_s32r(size_t offset, int32_t v);
  void pput_u48r(size_t offset, uint64_t v);
  void pput_s48r(size_t offset, int64_t v);
  void pput_u64r(size_t offset, uint64_t v);
  void pput_s64r(size_t offset, int64_t v);

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
