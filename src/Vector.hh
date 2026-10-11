#pragma once

#include <format>
#include <string>

namespace phosg {

template <typename ElemT, size_t Count>
struct Vector {
  std::array<ElemT, Count> data;

  constexpr Vector() : data{} {}
  constexpr Vector(std::initializer_list<ElemT> elements) {
    if (elements.size() != Count) {
      throw std::invalid_argument("Incorrect element count");
    }
    size_t z = 0;
    for (const auto& element : elements) {
      this->data[z++] = element;
    }
  }

  template <typename... ArgTs> requires((sizeof...(ArgTs) == Count) && (std::constructible_from<ElemT, ArgTs &&> && ...))
  constexpr Vector(ArgTs&&... args) : data{std::forward<ArgTs>(args)...} {}

  Vector(const Vector& other) = default;
  Vector(Vector&& other) = default;
  Vector& operator=(const Vector& other) = default;
  Vector& operator=(Vector&& other) = default;
  ~Vector() = default;

  // Elements accessible as:
  //   (x, y), (x, y, z), or (x, y, z, w) if Count in [2, 4]
  //   (r, g, b) if Count == 3
  ElemT& x() noexcept requires(Count >= 1) { return this->data[0]; }
  const ElemT& x() const noexcept requires(Count >= 1) { return this->data[0]; }
  ElemT& y() noexcept requires(Count >= 2) { return this->data[1]; }
  const ElemT& y() const noexcept requires(Count >= 2) { return this->data[1]; }
  ElemT& z() noexcept requires(Count >= 3) { return this->data[2]; }
  const ElemT& z() const noexcept requires(Count >= 3) { return this->data[2]; }
  ElemT& w() noexcept requires(Count >= 4) { return this->data[3]; }
  const ElemT& w() const noexcept requires(Count >= 4) { return this->data[3]; }

  ElemT& r() noexcept requires(Count == 3) { return this->data[0]; }
  const ElemT& r() const noexcept requires(Count == 3) { return this->data[0]; }
  ElemT& g() noexcept requires(Count == 3) { return this->data[1]; }
  const ElemT& g() const noexcept requires(Count == 3) { return this->data[1]; }
  ElemT& b() noexcept requires(Count == 3) { return this->data[2]; }
  const ElemT& b() const noexcept requires(Count == 3) { return this->data[2]; }

  template <typename RetT, typename FnT>
  requires(std::is_invocable_r_v<RetT, FnT, RetT, ElemT, size_t>)
  std::invoke_result_t<FnT, RetT, ElemT, size_t> reduce_elements(FnT&& fn, RetT value) const {
    for (size_t z = 0; z < Count; z++) {
      value = fn(value, this->data[z], z);
    }
    return value;
  }
  template <typename FnT>
  requires(std::is_invocable_r_v<ElemT, FnT, ElemT, size_t>)
  void transform_elements(FnT&& fn) {
    for (size_t z = 0; z < Count; z++) {
      this->data[z] = fn(this->data[z], z);
    }
  }
  template <typename FnT>
  requires(std::is_invocable_r_v<ElemT, FnT, ElemT, size_t>)
  Vector<ElemT, Count> copy_with_transform(FnT&& fn) const {
    Vector<ElemT, Count> ret = *this;
    ret.transform_elements(std::forward<FnT>(fn));
    return ret;
  }

  Vector<ElemT, Count> operator-() const {
    return this->copy_with_transform([](ElemT v, size_t) -> ElemT { return -v; });
  }

  Vector<ElemT, Count> operator+(const Vector<ElemT, Count>& other) const {
    return this->copy_with_transform([&other](ElemT v, size_t z) -> ElemT { return v + other.data[z]; });
  }
  Vector<ElemT, Count> operator-(const Vector<ElemT, Count>& other) const {
    return this->copy_with_transform([&other](ElemT v, size_t z) -> ElemT { return v - other.data[z]; });
  }
  Vector<ElemT, Count> operator+(ElemT other) const {
    return this->copy_with_transform([&other](ElemT v, size_t) -> ElemT { return v + other; });
  }
  Vector<ElemT, Count> operator-(ElemT other) const {
    return this->copy_with_transform([&other](ElemT v, size_t) -> ElemT { return v - other; });
  }
  Vector<ElemT, Count> operator*(ElemT other) const {
    return this->copy_with_transform([&other](ElemT v, size_t) -> ElemT { return v * other; });
  }
  Vector<ElemT, Count> operator/(ElemT other) const {
    return this->copy_with_transform([&other](ElemT v, size_t) -> ElemT { return v / other; });
  }
  Vector<ElemT, Count> operator%(ElemT other) const {
    return this->copy_with_transform([&other](ElemT v, size_t) -> ElemT { return v % other; });
  }
  Vector<ElemT, Count>& operator+=(const Vector<ElemT, Count>& other) {
    this->transform_elements([&other](ElemT v, size_t z) -> ElemT { return v + other.data[z]; });
    return *this;
  }
  Vector<ElemT, Count>& operator-=(const Vector<ElemT, Count>& other) {
    this->transform_elements([&other](ElemT v, size_t z) -> ElemT { return v - other.data[z]; });
    return *this;
  }
  Vector<ElemT, Count>& operator+=(ElemT other) {
    this->transform_elements([&other](ElemT v, size_t) -> ElemT { return v + other; });
    return *this;
  }
  Vector<ElemT, Count>& operator-=(ElemT other) {
    this->transform_elements([&other](ElemT v, size_t) -> ElemT { return v - other; });
    return *this;
  }
  Vector<ElemT, Count>& operator*=(ElemT other) {
    this->transform_elements([&other](ElemT v, size_t) -> ElemT { return v * other; });
    return *this;
  }
  Vector<ElemT, Count>& operator/=(ElemT other) {
    this->transform_elements([&other](ElemT v, size_t) -> ElemT { return v / other; });
    return *this;
  }
  Vector<ElemT, Count>& operator%=(ElemT other) {
    this->transform_elements([&other](ElemT v, size_t) -> ElemT { return v % other; });
    return *this;
  }

  bool operator!() const {
    for (size_t z = 0; z < Count; z++) {
      if (this->data[z] != 0) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const Vector<ElemT, Count>& other) const = default;
  bool operator!=(const Vector<ElemT, Count>& other) const = default;

  // For usage in set/map
  bool operator<(const Vector<ElemT, Count>& other) const {
    for (size_t z = 0; z < Count; z++) {
      if (this->data[z] < other.data[z]) {
        return true;
      } else if (this->data[z] > other.data[z]) {
        return false;
      }
    }
    return false;
  }

  ElemT at(size_t dim) const {
    return this->data.at(dim);
  }

  ElemT norm1() const {
    return this->reduce_elements<ElemT>([](ElemT sum, ElemT v, size_t) -> ElemT { return sum + v; }, 0);
  }
  double norm() const {
    return sqrt(this->norm2());
  }
  ElemT norm2() const {
    return this->reduce_elements<ElemT>([](ElemT sum, ElemT v, size_t) -> ElemT { return sum + (v * v); }, 0);
  }
  ElemT dot(const Vector<ElemT, Count>& other) const {
    return this->reduce_elements<ElemT>([&other](ElemT sum, ElemT v, size_t z) -> ElemT { return sum + (v * other.data[z]); }, 0);
  }

  Vector<ElemT, 3> cross(const Vector<ElemT, 3>& other) const
      requires(Count == 3)
  {
    return Vector<ElemT, 3>(
        this->y() * other.z() - this->z() * other.y(),
        this->z() * other.x() - this->x() * other.z(),
        this->x() * other.y() - this->y() * other.x());
  }

  std::string str() const {
    std::string ret = std::format("Vector{}[", Count);
    for (size_t z = 0; z < Count; z++) {
      ret += (z == 0) ? std::format("{}", this->data[z]) : std::format(", {}", this->data[z]);
    }
    ret.push_back(']');
    return ret;
  }

  static constexpr size_t dimensions() {
    return Count;
  }
};

template <typename ElemT>
using Vector2 = Vector<ElemT, 2>;
template <typename ElemT>
using Vector3 = Vector<ElemT, 3>;
template <typename ElemT>
using Vector4 = Vector<ElemT, 4>;

} // namespace phosg
