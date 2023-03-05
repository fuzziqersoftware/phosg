#include <utility>

template <typename IntT>
IntT gcd(IntT a, IntT b) {
  if (b == 0) {
    return a;
  }
  return gcd(b, a % b);
}

template <typename IntT>
std::pair<IntT, IntT> reduce_fraction(IntT a, IntT b) {
  IntT denom = gcd(a, b);
  return std::make_pair(a / denom, b / denom);
}
