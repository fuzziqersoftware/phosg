#pragma once

#include <stdint.h>

#include <string>


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

template <typename T>
T bswap(T) {
  static_assert(always_false<T>::v, "unspecialized bswap should never be called");
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

template <typename FromT, typename ToT>
ToT bswapf(FromT) {
  static_assert(always_false<FromT, ToT>::v, "unspecialized bswapf should never be called");
}

template <>
inline uint32_t bswapf<float, uint32_t>(float v) {
  return bswap32f(v);
}

template <>
inline float bswapf<uint32_t, float>(uint32_t v) {
  return bswap32f(v);
}

template <>
inline uint64_t bswapf<double, uint64_t>(double v) {
  return bswap64f(v);
}

template <>
inline double bswapf<uint64_t, double>(uint64_t v) {
  return bswap64f(v);
}



template <typename T>
class reverse_endian {
private:
  T value;

public:
  reverse_endian() = default;
  reverse_endian(T v) : value(bswap<T>(v)) { }
  reverse_endian(const reverse_endian<T>& other) = default;
  reverse_endian(reverse_endian<T>&& other) = default;
  reverse_endian& operator=(const reverse_endian<T>& other) = default;
  reverse_endian& operator=(reverse_endian<T>&& other) = default;

  // Access operators
  operator T() const {
    return bswap<T>(this->value);
  }
  void store(T v) {
    this->value = bswap<T>(v);
  }
  T load() const {
    return bswap<T>(this->value);
  }
  void store_raw(T v) {
    this->value = v;
  }
  T load_raw() const {
    return this->value;
  }

  // Assignment operators
  reverse_endian& operator=(T v) {
    this->value = bswap<T>(v);
    return *this;
  };

  // Arithmetic assignment operators
  template <typename R>
  reverse_endian& operator+=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) + delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator-=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) - delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator*=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) * delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator/=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) / delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator%=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) % delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator&=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) & delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator|=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) | delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator^=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) ^ delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator<<=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) << delta);
    return *this;
  }
  template <typename R>
  reverse_endian& operator>>=(R delta) {
    this->value = bswap<T>(bswap<T>(this->value) >> delta);
    return *this;
  }
  T operator++() {
    this->value = bswap<T>(bswap<T>(this->value) + 1);
    return this->value;
  }
  T operator--() {
    this->value = bswap<T>(bswap<T>(this->value) - 1);
    return this->value;
  }
  T operator++(int) {
    T ret = bswap<T>(this->value);
    this->value = bswap<T>(ret + 1);
    return ret;
  }
  T operator--(int) {
    T ret = bswap<T>(this->value);
    this->value = bswap<T>(ret - 1);
    return ret;
  }
} __attribute__((packed));

using re_uint16_t = reverse_endian<uint16_t>;
using re_int16_t = reverse_endian<int16_t>;
using re_uint32_t = reverse_endian<uint32_t>;
using re_int32_t = reverse_endian<int32_t>;
using re_uint64_t = reverse_endian<uint64_t>;
using re_int64_t = reverse_endian<int64_t>;



template <typename ExposedT, typename StoredT>
class reverse_endian_float {
private:
  static_assert(sizeof(ExposedT) == sizeof(StoredT), "ExposedT and StoredT must be the same size");
  StoredT value;

public:
  reverse_endian_float() { }
  reverse_endian_float(ExposedT v) : value(bswapf<ExposedT, StoredT>(v)) { }
  reverse_endian_float& operator=(ExposedT v) {
    this->value = bswapf<ExposedT, StoredT>(v);
    return *this;
  };
  operator ExposedT() const {
    return bswapf<StoredT, ExposedT>(this->value);
  }
  void store_raw(StoredT v) {
    this->value = v;
  }
  StoredT load_raw() const {
    return this->value;
  }
} __attribute__((packed));

using re_float = reverse_endian_float<float, uint32_t>;
using re_double = reverse_endian_float<double, uint64_t>;



extern const char* DEFAULT_ALPHABET;
extern const char* URLSAFE_ALPHABET;

std::string base64_encode(const void* data, size_t size,
    const char* alphabet = nullptr);
std::string base64_encode(const std::string& data, const char* alphabet = nullptr);
std::string base64_decode(const void* data, size_t size,
    const char* alphabet = nullptr);
std::string base64_decode(const std::string& data, const char* alphabet = nullptr);

std::string rot13(const void* data, size_t size);
