#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Encoding.hh"

namespace phosg {

template <typename IntT>
constexpr IntT gcd(IntT a, IntT b) {
  while (b != 0) {
    IntT m = a % b;
    a = b;
    b = m;
  }
  return a;
}

template <typename IntT>
constexpr std::pair<IntT, IntT> reduce_fraction(IntT a, IntT b) {
  IntT denom = gcd(a, b);
  return std::make_pair(a / denom, b / denom);
}

constexpr uint8_t log2i(uint64_t v) {
  return (v == 0) ? 64 : (63 - __builtin_clzll(v));
}

} // namespace phosg
