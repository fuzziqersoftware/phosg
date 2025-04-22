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

template <typename IntT>
constexpr IntT log2i(IntT v) {
  return (sizeof(IntT) << 3) - 1 - __builtin_clz(v);
}

} // namespace phosg
