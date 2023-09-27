#pragma once

#include <inttypes.h>
#include <stdint.h>

#include <string>

#include "Platform.hh"
#include "Types.hh"

template <typename T>
constexpr uint8_t bits_for_type = sizeof(T) << 3;

template <typename T>
constexpr T msb_for_type = (static_cast<T>(1) << (bits_for_type<T> - 1));

template <typename T>
constexpr uint64_t mask_for_type = 0xFFFFFFFFFFFFFFFF >> (64 - bits_for_type<T>);

template <typename T>
constexpr const char* printf_format_for_type() {
  static_assert(always_false<T>::v,
      "unspecialized printf_format_for_type should never be called");
  return nullptr;
}

template <typename T>
constexpr const char* printf_hex_format_for_type() {
  static_assert(always_false<T>::v,
      "unspecialized printf_hex_format_for_type should never be called");
  return nullptr;
}

template <>
constexpr const char* printf_format_for_type<uint8_t>() { return PRIu8; }
template <>
constexpr const char* printf_format_for_type<int8_t>() { return PRId8; }
template <>
constexpr const char* printf_format_for_type<uint16_t>() { return PRIu16; }
template <>
constexpr const char* printf_format_for_type<int16_t>() { return PRId16; }
template <>
constexpr const char* printf_format_for_type<uint32_t>() { return PRIu32; }
template <>
constexpr const char* printf_format_for_type<int32_t>() { return PRId32; }
template <>
constexpr const char* printf_format_for_type<uint64_t>() { return PRIu64; }
template <>
constexpr const char* printf_format_for_type<int64_t>() { return PRId64; }

template <>
constexpr const char* printf_hex_format_for_type<uint8_t>() { return PRIX8; }
template <>
constexpr const char* printf_hex_format_for_type<int8_t>() { return PRIX8; }
template <>
constexpr const char* printf_hex_format_for_type<uint16_t>() { return PRIX16; }
template <>
constexpr const char* printf_hex_format_for_type<int16_t>() { return PRIX16; }
template <>
constexpr const char* printf_hex_format_for_type<uint32_t>() { return PRIX32; }
template <>
constexpr const char* printf_hex_format_for_type<int32_t>() { return PRIX32; }
template <>
constexpr const char* printf_hex_format_for_type<uint64_t>() { return PRIX64; }
template <>
constexpr const char* printf_hex_format_for_type<int64_t>() { return PRIX64; }

template <typename ResultT, typename SrcT>
ResultT sign_extend(SrcT src) {
  using UResultT = std::make_unsigned_t<ResultT>;
  if (src & (1 << ((sizeof(SrcT) << 3) - 1))) {
    return static_cast<ResultT>(src) | (static_cast<ResultT>(static_cast<UResultT>(-1) << (sizeof(SrcT) << 3)));
  } else {
    return static_cast<ResultT>(src);
  }
}

static inline int32_t ext24(uint32_t a) {
  return (a & 0x00800000) ? (a | 0xFF000000) : a;
}

static inline int64_t ext48(uint64_t a) {
  return (a & 0x00008000000000) ? (a | 0xFFFF0000000000) : a;
}

static inline uint8_t bswap8(uint8_t a) {
  return a;
}

static inline uint16_t bswap16(uint16_t a) {
  return ((a >> 8) & 0x00FF) |
      ((a << 8) & 0xFF00);
}

static inline uint32_t bswap24(uint32_t a) {
  return ((a >> 16) & 0x000000FF) |
      (a & 0x0000FF00) |
      ((a << 16) & 0x00FF0000);
}

static inline int32_t bswap24s(int32_t a) {
  int32_t r = bswap24(a);
  if (r & 0x00800000) {
    return r | 0xFF000000;
  }
  return r;
}

static inline uint32_t bswap32(uint32_t a) {
  return ((a >> 24) & 0x000000FF) |
      ((a >> 8) & 0x0000FF00) |
      ((a << 8) & 0x00FF0000) |
      ((a << 24) & 0xFF000000);
}

static inline uint64_t bswap48(uint64_t a) {
  return ((a >> 40) & 0x00000000000000FF) |
      ((a >> 24) & 0x000000000000FF00) |
      ((a >> 8) & 0x0000000000FF0000) |
      ((a << 8) & 0x00000000FF000000) |
      ((a << 24) & 0x000000FF00000000) |
      ((a << 40) & 0x0000FF0000000000);
}

static inline int64_t bswap48s(int64_t a) {
  int64_t r = bswap48(a);
  if (r & 0x0000800000000000) {
    return r | 0xFFFF000000000000;
  }
  return r;
}

static inline uint64_t bswap64(uint64_t a) {
  return ((a >> 56) & 0x00000000000000FF) |
      ((a >> 40) & 0x000000000000FF00) |
      ((a >> 24) & 0x0000000000FF0000) |
      ((a >> 8) & 0x00000000FF000000) |
      ((a << 8) & 0x000000FF00000000) |
      ((a << 24) & 0x0000FF0000000000) |
      ((a << 40) & 0x00FF000000000000) |
      ((a << 56) & 0xFF00000000000000);
}

static inline float bswap32f(uint32_t a) {
  float f;
  *(uint32_t*)(&f) = bswap32(a);
  return f;
}

static inline double bswap64f(uint64_t a) {
  double d;
  *(uint64_t*)(&d) = bswap64(a);
  return d;
}

static inline uint32_t bswap32f(float a) {
  uint32_t i = *(uint32_t*)(&a);
  return bswap32(i);
}

static inline uint64_t bswap64f(double a) {
  uint64_t i = *(uint64_t*)(&a);
  return bswap64(i);
}

template <typename ArgT, typename ResultT = ArgT>
ResultT bswap(ArgT) {
  static_assert(always_false<ArgT, ResultT>::v,
      "unspecialized bswap should never be called");
}

template <>
inline uint8_t bswap<uint8_t>(uint8_t v) {
  return v;
}

template <>
inline int8_t bswap<int8_t>(int8_t v) {
  return v;
}

template <>
inline uint16_t bswap<uint16_t>(uint16_t v) {
  return bswap16(v);
}

template <>
inline int16_t bswap<int16_t>(int16_t v) {
  return bswap16(v);
}

template <>
inline uint32_t bswap<uint32_t>(uint32_t v) {
  return bswap32(v);
}

template <>
inline int32_t bswap<int32_t>(int32_t v) {
  return bswap32(v);
}

template <>
inline uint64_t bswap<uint64_t>(uint64_t v) {
  return bswap64(v);
}

template <>
inline int64_t bswap<int64_t>(int64_t v) {
  return bswap64(v);
}

template <>
inline uint32_t bswap<float, uint32_t>(float v) {
  return bswap32f(v);
}

template <>
inline float bswap<uint32_t, float>(uint32_t v) {
  return bswap32f(v);
}

template <>
inline uint64_t bswap<double, uint64_t>(double v) {
  return bswap64f(v);
}

template <>
inline double bswap<uint64_t, double>(uint64_t v) {
  return bswap64f(v);
}

template <typename ArgT, typename ResultT = ArgT>
struct bswap_st {
  static inline ResultT fn(ArgT v) {
    return bswap<ArgT, ResultT>(v);
  }
};

template <typename ArgT, typename ResultT = ArgT>
struct ident_st {
  static inline ResultT fn(ArgT v) {
    return *reinterpret_cast<const ResultT*>(&v);
  }
};

template <typename ExposedT, typename StoredT, typename OnStoreSt, typename OnLoadSt>
class converted_endian {
private:
  StoredT value;

public:
  converted_endian() = default;
  converted_endian(ExposedT v) : value(OnStoreSt::fn(v)) {}
  converted_endian(const converted_endian& other) = default;
  converted_endian(converted_endian&& other) = default;
  converted_endian& operator=(const converted_endian& other) = default;
  converted_endian& operator=(converted_endian&& other) = default;

  // Access operators
  operator ExposedT() const {
    return OnLoadSt::fn(this->value);
  }
  void store(ExposedT v) {
    this->value = OnStoreSt::fn(v);
  }
  ExposedT load() const {
    return OnLoadSt::fn(this->value);
  }
  void store_raw(StoredT v) {
    this->value = v;
  }
  StoredT load_raw() const {
    return this->value;
  }

  // Assignment operators
  converted_endian& operator=(ExposedT v) {
    this->value = OnStoreSt::fn(v);
    return *this;
  };

  // Arithmetic assignment operators
  template <typename R>
  converted_endian& operator+=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) + delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator-=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) - delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator*=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) * delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator/=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) / delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator%=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) % delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator&=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) & delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator|=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) | delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator^=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) ^ delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator<<=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) << delta);
    return *this;
  }
  template <typename R>
  converted_endian& operator>>=(R delta) {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) >> delta);
    return *this;
  }
  ExposedT operator++() {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) + 1);
    return this->value;
  }
  ExposedT operator--() {
    this->value = OnStoreSt::fn(OnLoadSt::fn(this->value) - 1);
    return this->value;
  }
  ExposedT operator++(int) {
    ExposedT ret = OnLoadSt::fn(this->value);
    this->value = OnStoreSt::fn(ret + 1);
    return ret;
  }
  ExposedT operator--(int) {
    ExposedT ret = OnLoadSt::fn(this->value);
    this->value = OnStoreSt::fn(ret - 1);
    return ret;
  }
} __attribute__((packed));

template <typename ExposedT, typename StoredT = ExposedT>
class reverse_endian
    : public converted_endian<ExposedT, StoredT, bswap_st<ExposedT, StoredT>, bswap_st<StoredT, ExposedT>> {
public:
  using converted_endian<ExposedT, StoredT, bswap_st<ExposedT, StoredT>, bswap_st<StoredT, ExposedT>>::converted_endian;
} __attribute__((packed));

template <typename ExposedT, typename StoredT = ExposedT>
class same_endian
    : public converted_endian<ExposedT, StoredT, ident_st<ExposedT, StoredT>, ident_st<StoredT, ExposedT>> {
public:
  using converted_endian<ExposedT, StoredT, ident_st<ExposedT, StoredT>, ident_st<StoredT, ExposedT>>::converted_endian;
} __attribute__((packed));

#ifdef PHOSG_LITTLE_ENDIAN

template <typename ExposedT, typename StoredT = ExposedT>
class big_endian : public reverse_endian<ExposedT, StoredT> {
public:
  using reverse_endian<ExposedT, StoredT>::reverse_endian;
} __attribute__((packed));

template <typename ExposedT, typename StoredT = ExposedT>
class little_endian : public same_endian<ExposedT, StoredT> {
public:
  using same_endian<ExposedT, StoredT>::same_endian;
} __attribute__((packed));

#elif defined(PHOSG_BIG_ENDIAN)

template <typename ExposedT, typename StoredT = ExposedT>
class little_endian : public reverse_endian<ExposedT, StoredT> {
public:
  using reverse_endian<ExposedT, StoredT>::reverse_endian;
} __attribute__((packed));

template <typename ExposedT, typename StoredT = ExposedT>
class big_endian : public same_endian<ExposedT, StoredT> {
public:
  using same_endian<ExposedT, StoredT>::same_endian;
} __attribute__((packed));

#else

#error "No endianness define exists"

#endif

using re_uint16_t = reverse_endian<uint16_t>;
using re_int16_t = reverse_endian<int16_t>;
using re_uint32_t = reverse_endian<uint32_t>;
using re_int32_t = reverse_endian<int32_t>;
using re_uint64_t = reverse_endian<uint64_t>;
using re_int64_t = reverse_endian<int64_t>;
using re_float = reverse_endian<float, uint32_t>;
using re_double = reverse_endian<double, uint64_t>;
using le_uint16_t = little_endian<uint16_t>;
using le_int16_t = little_endian<int16_t>;
using le_uint32_t = little_endian<uint32_t>;
using le_int32_t = little_endian<int32_t>;
using le_uint64_t = little_endian<uint64_t>;
using le_int64_t = little_endian<int64_t>;
using le_float = little_endian<float, uint32_t>;
using le_double = little_endian<double, uint64_t>;
using be_uint16_t = big_endian<uint16_t>;
using be_int16_t = big_endian<int16_t>;
using be_uint32_t = big_endian<uint32_t>;
using be_int32_t = big_endian<int32_t>;
using be_uint64_t = big_endian<uint64_t>;
using be_int64_t = big_endian<int64_t>;
using be_float = big_endian<float, uint32_t>;
using be_double = big_endian<double, uint64_t>;

template <typename T>
constexpr bool is_converted_endian_int_sc_v =
    std::is_same_v<T, le_uint16_t> ||
    std::is_same_v<T, le_uint32_t> ||
    std::is_same_v<T, le_uint64_t> ||
    std::is_same_v<T, le_int16_t> ||
    std::is_same_v<T, le_int32_t> ||
    std::is_same_v<T, le_int64_t> ||
    std::is_same_v<T, be_uint16_t> ||
    std::is_same_v<T, be_uint32_t> ||
    std::is_same_v<T, be_uint64_t> ||
    std::is_same_v<T, be_int16_t> ||
    std::is_same_v<T, be_int32_t> ||
    std::is_same_v<T, be_int64_t> ||
    std::is_same_v<T, re_uint16_t> ||
    std::is_same_v<T, re_uint32_t> ||
    std::is_same_v<T, re_uint64_t> ||
    std::is_same_v<T, re_int16_t> ||
    std::is_same_v<T, re_int32_t> ||
    std::is_same_v<T, re_int64_t>;

template <typename T>
constexpr bool is_converted_endian_float_sc_v =
    std::is_same_v<T, le_float> ||
    std::is_same_v<T, le_double> ||
    std::is_same_v<T, be_float> ||
    std::is_same_v<T, be_double> ||
    std::is_same_v<T, re_float> ||
    std::is_same_v<T, re_double>;

template <typename T>
constexpr bool is_converted_endian_sc_v = is_converted_endian_int_sc_v<T> || is_converted_endian_float_sc_v<T>;

extern const char* DEFAULT_ALPHABET;
extern const char* URLSAFE_ALPHABET;

std::string base64_encode(const void* data, size_t size, const char* alphabet = nullptr);
std::string base64_encode(const std::string& data, const char* alphabet = nullptr);
std::string base64_decode(const void* data, size_t size, const char* alphabet = nullptr);
std::string base64_decode(const std::string& data, const char* alphabet = nullptr);

std::string rot13(const void* data, size_t size);
