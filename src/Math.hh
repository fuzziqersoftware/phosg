#include <utility>

template <typename IntT>
constexpr IntT gcd(IntT a, IntT b) {
  if (b == 0) {
    return a;
  }
  return gcd(b, a % b);
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
