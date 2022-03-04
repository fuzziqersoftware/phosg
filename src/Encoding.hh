#pragma once

#include <stdint.h>

#include <string>

#include "Platform.hh"


static inline uint8_t bswap8(uint8_t a) {
  return a;
}

static inline uint16_t bswap16(uint16_t a) {
  return ((a >> 8) & 0x00FF) |
         ((a << 8) & 0xFF00);
}

static inline uint32_t bswap24(uint32_t a) {
  return ((a >> 16) & 0x000000FF) |
         ( a        & 0x0000FF00) |
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
         ((a >> 8)  & 0x0000FF00) |
         ((a << 8)  & 0x00FF0000) |
         ((a << 24) & 0xFF000000);
}

static inline uint64_t bswap48(uint64_t a) {
  return ((a >> 40) & 0x00000000000000FF) |
         ((a >> 24) & 0x000000000000FF00) |
         ((a >> 8)  & 0x0000000000FF0000) |
         ((a << 8)  & 0x00000000FF000000) |
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
         ((a >> 8)  & 0x00000000FF000000) |
         ((a << 8)  & 0x000000FF00000000) |
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



template <typename...>
struct always_false {
  static constexpr bool v = false;
};

template <typename ArgT, typename ResultT = ArgT>
ResultT bswap(ArgT) {
  static_assert(always_false<ArgT, ResultT>::v,
      "unspecialized bswap should never be called");
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
    return v;
  }
};



template <typename ExposedT, typename StoredT, typename OnStoreSt, typename OnLoadSt>
class converted_endian {
private:
  StoredT value;

public:
  converted_endian() = default;
  converted_endian(ExposedT v) : value(OnStoreSt::fn(v)) { }
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
};

template <typename ExposedT, typename StoredT = ExposedT>
class same_endian
  : public converted_endian<ExposedT, StoredT, ident_st<ExposedT, StoredT>, ident_st<StoredT, ExposedT>> {
public:
  using converted_endian<ExposedT, StoredT, ident_st<ExposedT, StoredT>, ident_st<StoredT, ExposedT>>::converted_endian;
};



using re_uint16_t = reverse_endian<uint16_t>;
using re_int16_t = reverse_endian<int16_t>;
using re_uint32_t = reverse_endian<uint32_t>;
using re_int32_t = reverse_endian<int32_t>;
using re_uint64_t = reverse_endian<uint64_t>;
using re_int64_t = reverse_endian<int64_t>;
using re_float = reverse_endian<float, uint32_t>;
using re_double = reverse_endian<double, uint64_t>;

#ifdef PHOSG_LITTLE_ENDIAN

using le_uint16_t = same_endian<uint16_t>;
using le_int16_t = same_endian<int16_t>;
using le_uint32_t = same_endian<uint32_t>;
using le_int32_t = same_endian<int32_t>;
using le_uint64_t = same_endian<uint64_t>;
using le_int64_t = same_endian<int64_t>;
using be_uint16_t = reverse_endian<uint16_t>;
using be_int16_t = reverse_endian<int16_t>;
using be_uint32_t = reverse_endian<uint32_t>;
using be_int32_t = reverse_endian<int32_t>;
using be_uint64_t = reverse_endian<uint64_t>;
using be_int64_t = reverse_endian<int64_t>;

#elif defined(PHOSG_BIG_ENDIAN)

using le_uint16_t = reverse_endian<uint16_t>;
using le_int16_t = reverse_endian<int16_t>;
using le_uint32_t = reverse_endian<uint32_t>;
using le_int32_t = reverse_endian<int32_t>;
using le_uint64_t = reverse_endian<uint64_t>;
using le_int64_t = reverse_endian<int64_t>;
using be_uint16_t = same_endian<uint16_t>;
using be_int16_t = same_endian<int16_t>;
using be_uint32_t = same_endian<uint32_t>;
using be_int32_t = same_endian<int32_t>;
using be_uint64_t = same_endian<uint64_t>;
using be_int64_t = same_endian<int64_t>;

#else

#error "No endianness define exists"

#endif



// TODO: implement the above endian-converted types for floats



extern const char* DEFAULT_ALPHABET;
extern const char* URLSAFE_ALPHABET;

std::string base64_encode(const void* data, size_t size,
    const char* alphabet = nullptr);
std::string base64_encode(const std::string& data, const char* alphabet = nullptr);
std::string base64_decode(const void* data, size_t size,
    const char* alphabet = nullptr);
std::string base64_decode(const std::string& data, const char* alphabet = nullptr);

std::string rot13(const void* data, size_t size);
