#pragma once

#include <stdint.h>


static inline uint8_t bswap8(uint8_t a) {
  return a;
}

static inline uint64_t bswap16(uint16_t a) {
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
