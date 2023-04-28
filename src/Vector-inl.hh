#include "Vector.hh"

#include <math.h>

#include <stdexcept>

#include "Strings.hh"

template <typename T>
Vector2<T>::Vector2()
    : x(0),
      y(0) {}

template <typename T>
Vector2<T>::Vector2(T x, T y)
    : x(x),
      y(y) {}

template <typename T>
Vector2<T> Vector2<T>::operator-() const {
  return Vector2(-this->x, -this->y);
}

template <typename T>
Vector2<T> Vector2<T>::operator+(const Vector2<T>& other) const {
  return Vector2(this->x + other.x, this->y + other.y);
}

template <typename T>
Vector2<T> Vector2<T>::operator-(const Vector2<T>& other) const {
  return Vector2(this->x - other.x, this->y - other.y);
}

template <typename T>
Vector2<T> Vector2<T>::operator+(T other) const {
  return Vector2(this->x + other, this->y + other);
}

template <typename T>
Vector2<T> Vector2<T>::operator-(T other) const {
  return Vector2(this->x - other, this->y - other);
}

template <typename T>
Vector2<T> Vector2<T>::operator*(T other) const {
  return Vector2(this->x * other, this->y * other);
}

template <typename T>
Vector2<T> Vector2<T>::operator/(T other) const {
  return Vector2(this->x / other, this->y / other);
}

template <typename T>
Vector2<T> Vector2<T>::operator%(T other) const {
  return Vector2(this->x % other, this->y % other);
}

template <typename T>
Vector2<T>& Vector2<T>::operator+=(const Vector2<T>& other) {
  this->x += other.x;
  this->y += other.y;
  return *this;
}

template <typename T>
Vector2<T>& Vector2<T>::operator-=(const Vector2<T>& other) {
  this->x -= other.x;
  this->y -= other.y;
  return *this;
}

template <typename T>
Vector2<T>& Vector2<T>::operator+=(T other) {
  this->x += other;
  this->y += other;
  return *this;
}

template <typename T>
Vector2<T>& Vector2<T>::operator-=(T other) {
  this->x -= other;
  this->y -= other;
  return *this;
}

template <typename T>
Vector2<T>& Vector2<T>::operator*=(T other) {
  this->x *= other;
  this->y *= other;
  return *this;
}

template <typename T>
Vector2<T>& Vector2<T>::operator/=(T other) {
  this->x /= other;
  this->y /= other;
  return *this;
}

template <typename T>
Vector2<T>& Vector2<T>::operator%=(T other) {
  this->x %= other;
  this->y %= other;
  return *this;
}

template <typename T>
bool Vector2<T>::operator!() const {
  return (this->x == 0) && (this->y == 0);
}

template <typename T>
bool Vector2<T>::operator==(const Vector2<T>& other) const {
  return (this->x == other.x) && (this->y == other.y);
}

template <typename T>
bool Vector2<T>::operator!=(const Vector2<T>& other) const {
  return !(this->operator==(other));
}

template <typename T>
bool Vector2<T>::operator<(const Vector2<T>& other) const {
  if (this->x < other.x) {
    return true;
  }
  if (this->x > other.x) {
    return false;
  }
  return this->y < other.y;
}

template <typename T>
T Vector2<T>::at(size_t dim) const {
  return reinterpret_cast<const T*>(this)[dim];
}

template <typename T>
T Vector2<T>::norm1() const {
  return this->x + this->y;
}

template <typename T>
double Vector2<T>::norm() const {
  return sqrt(this->norm2());
}

template <typename T>
T Vector2<T>::norm2() const {
  return (this->x * this->x) + (this->y * this->y);
}

template <typename T>
T Vector2<T>::dot(const Vector2<T>& other) const {
  return (this->x * other.x) + (this->y * other.y);
}

template <typename T>
std::string Vector2<T>::str() const {
  return string_printf("[%g, %g]", this->x, this->y);
}

template <typename T>
constexpr size_t Vector2<T>::dimensions() {
  return 2;
}

template <typename T>
Vector3<T>::Vector3()
    : x(0),
      y(0),
      z(0) {}

template <typename T>
Vector3<T>::Vector3(T x, T y, T z)
    : x(x),
      y(y),
      z(z) {}

template <typename T>
Vector3<T>::Vector3(const Vector2<T>& xy, T z)
    : x(xy.x),
      y(xy.y),
      z(z) {}

template <typename T>
Vector3<T> Vector3<T>::operator-() const {
  return Vector3(-this->x, -this->y, -this->z);
}

template <typename T>
Vector3<T> Vector3<T>::operator+(const Vector3<T>& other) const {
  return Vector3(this->x + other.x, this->y + other.y, this->z + other.z);
}

template <typename T>
Vector3<T> Vector3<T>::operator-(const Vector3<T>& other) const {
  return Vector3(this->x - other.x, this->y - other.y, this->z - other.z);
}

template <typename T>
Vector3<T> Vector3<T>::operator+(T other) const {
  return Vector3(this->x + other, this->y + other, this->z + other);
}

template <typename T>
Vector3<T> Vector3<T>::operator-(T other) const {
  return Vector3(this->x - other, this->y - other, this->z - other);
}

template <typename T>
Vector3<T> Vector3<T>::operator*(T other) const {
  return Vector3(this->x * other, this->y * other, this->z * other);
}

template <typename T>
Vector3<T> Vector3<T>::operator/(T other) const {
  return Vector3(this->x / other, this->y / other, this->z / other);
}

template <typename T>
Vector3<T> Vector3<T>::operator%(T other) const {
  return Vector3(this->x % other, this->y % other, this->z % other);
}

template <typename T>
Vector3<T>& Vector3<T>::operator+=(const Vector3<T>& other) {
  this->x += other.x;
  this->y += other.y;
  this->z += other.z;
  return *this;
}

template <typename T>
Vector3<T>& Vector3<T>::operator-=(const Vector3<T>& other) {
  this->x -= other.x;
  this->y -= other.y;
  this->z -= other.z;
  return *this;
}

template <typename T>
Vector3<T>& Vector3<T>::operator+=(T other) {
  this->x += other;
  this->y += other;
  this->z += other;
  return *this;
}

template <typename T>
Vector3<T>& Vector3<T>::operator-=(T other) {
  this->x -= other;
  this->y -= other;
  this->z -= other;
  return *this;
}

template <typename T>
Vector3<T>& Vector3<T>::operator*=(T other) {
  this->x *= other;
  this->y *= other;
  this->z *= other;
  return *this;
}

template <typename T>
Vector3<T>& Vector3<T>::operator/=(T other) {
  this->x /= other;
  this->y /= other;
  this->z /= other;
  return *this;
}

template <typename T>
Vector3<T>& Vector3<T>::operator%=(T other) {
  this->x %= other;
  this->y %= other;
  this->z %= other;
  return *this;
}

template <typename T>
bool Vector3<T>::operator!() const {
  return (this->x == 0) && (this->y == 0) && (this->z == 0);
}

template <typename T>
bool Vector3<T>::operator==(const Vector3<T>& other) const {
  return (this->x == other.x) && (this->y == other.y) && (this->z == other.z);
}

template <typename T>
bool Vector3<T>::operator!=(const Vector3<T>& other) const {
  return !(this->operator==(other));
}

template <typename T>
bool Vector3<T>::operator<(const Vector3<T>& other) const {
  if (this->x < other.x) {
    return true;
  }
  if (this->x > other.x) {
    return false;
  }
  if (this->y < other.y) {
    return true;
  }
  if (this->y > other.y) {
    return false;
  }
  return this->z < other.z;
}

template <typename T>
T Vector3<T>::at(size_t dim) const {
  return reinterpret_cast<const T*>(this)[dim];
}

template <typename T>
T Vector3<T>::norm1() const {
  return this->x + this->y + this->z;
}

template <typename T>
double Vector3<T>::norm() const {
  return sqrt(this->norm2());
}

template <typename T>
T Vector3<T>::norm2() const {
  return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
}

template <typename T>
T Vector3<T>::dot(const Vector3<T>& other) const {
  return (this->x * other.x) + (this->y * other.y) + (this->z * other.z);
}

template <typename T>
Vector3<T> Vector3<T>::cross(const Vector3<T>& other) const {
  return Vector3<T>(
      this->y * other.z - this->z * other.y,
      this->z * other.x - this->x * other.z,
      this->x * other.y - this->y * other.x);
}

template <typename T>
std::string Vector3<T>::str() const {
  return string_printf("[%g, %g, %g]", this->x, this->y, this->z);
}

template <typename T>
constexpr size_t Vector3<T>::dimensions() {
  return 3;
}

template <typename T>
Vector4<T>::Vector4()
    : x(0),
      y(0),
      z(0),
      w(0) {}

template <typename T>
Vector4<T>::Vector4(T x, T y, T z, T w)
    : x(x),
      y(y),
      z(z),
      w(w) {}

template <typename T>
Vector4<T>::Vector4(const Vector2<T>& xy, T z, T w)
    : x(xy.x),
      y(xy.y),
      z(z),
      w(w) {}

template <typename T>
Vector4<T>::Vector4(const Vector3<T>& xyz, T w)
    : x(xyz.x),
      y(xyz.y),
      z(xyz.z),
      w(w) {}

template <typename T>
Vector4<T> Vector4<T>::operator-() const {
  return Vector4<T>(-this->x, -this->y, -this->z, -this->w);
}

template <typename T>
Vector4<T> Vector4<T>::operator+(const Vector4<T>& other) const {
  return Vector4<T>(this->x + other.x, this->y + other.y, this->z + other.z,
      this->w + other.w);
}

template <typename T>
Vector4<T> Vector4<T>::operator-(const Vector4<T>& other) const {
  return Vector4<T>(this->x - other.x, this->y - other.y, this->z - other.z,
      this->w - other.w);
}

template <typename T>
Vector4<T> Vector4<T>::operator+(T other) const {
  return Vector4<T>(this->x + other, this->y + other, this->z + other,
      this->w + other);
}

template <typename T>
Vector4<T> Vector4<T>::operator-(T other) const {
  return Vector4<T>(this->x - other, this->y - other, this->z - other,
      this->w - other);
}

template <typename T>
Vector4<T> Vector4<T>::operator*(T other) const {
  return Vector4<T>(this->x * other, this->y * other, this->z * other,
      this->w * other);
}

template <typename T>
Vector4<T> Vector4<T>::operator/(T other) const {
  return Vector4<T>(this->x / other, this->y / other, this->z / other,
      this->w / other);
}

template <typename T>
Vector4<T> Vector4<T>::operator%(T other) const {
  return Vector4<T>(this->x % other, this->y % other, this->z % other,
      this->w % other);
}

template <typename T>
Vector4<T>& Vector4<T>::operator+=(const Vector4<T>& other) {
  this->x += other.x;
  this->y += other.y;
  this->z += other.z;
  this->w += other.w;
  return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator-=(const Vector4<T>& other) {
  this->x -= other.x;
  this->y -= other.y;
  this->z -= other.z;
  this->w -= other.w;
  return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator+=(T other) {
  this->x += other;
  this->y += other;
  this->z += other;
  this->w += other;
  return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator-=(T other) {
  this->x -= other;
  this->y -= other;
  this->z -= other;
  this->w -= other;
  return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator*=(T other) {
  this->x *= other;
  this->y *= other;
  this->z *= other;
  this->w *= other;
  return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator/=(T other) {
  this->x /= other;
  this->y /= other;
  this->z /= other;
  this->w /= other;
  return *this;
}

template <typename T>
Vector4<T>& Vector4<T>::operator%=(T other) {
  this->x %= other;
  this->y %= other;
  this->z %= other;
  this->w %= other;
  return *this;
}

template <typename T>
bool Vector4<T>::operator!() const {
  return (this->x == 0) && (this->y == 0) && (this->z == 0) && (this->w == 0);
}

template <typename T>
bool Vector4<T>::operator==(const Vector4<T>& other) const {
  return (this->x == other.x) && (this->y == other.y) && (this->z == other.z) &&
      (this->w == other.w);
}

template <typename T>
bool Vector4<T>::operator!=(const Vector4<T>& other) const {
  return !(this->operator==(other));
}

template <typename T>
bool Vector4<T>::operator<(const Vector4<T>& other) const {
  if (this->x < other.x) {
    return true;
  }
  if (this->x > other.x) {
    return false;
  }
  if (this->y < other.y) {
    return true;
  }
  if (this->y > other.y) {
    return false;
  }
  if (this->z < other.z) {
    return true;
  }
  if (this->z > other.z) {
    return false;
  }
  return this->w < other.w;
}

template <typename T>
T Vector4<T>::at(size_t dim) const {
  return reinterpret_cast<const T*>(this)[dim];
}

template <typename T>
T Vector4<T>::norm1() const {
  return this->x + this->y + this->z + this->w;
}

template <typename T>
double Vector4<T>::norm() const {
  return sqrt(this->norm2());
}

template <typename T>
T Vector4<T>::norm2() const {
  return (this->x * this->x) + (this->y * this->y) + (this->z * this->z) +
      (this->w * this->w);
}

template <typename T>
T Vector4<T>::dot(const Vector4<T>& other) const {
  return (this->x * other.x) + (this->y * other.y) + (this->z * other.z) +
      (this->w * other.w);
}

template <typename T>
std::string Vector4<T>::str() const {
  return string_printf("[%g, %g, %g, %g]", this->x, this->y, this->z, this->w);
}

template <typename T>
constexpr size_t Vector4<T>::dimensions() {
  return 4;
}

template <typename T>
Matrix4<T>::Matrix4() {
  for (size_t x = 0; x < 4; x++) {
    for (size_t y = 0; y < 4; y++) {
      this->m[x][y] = (y == x);
    }
  }
}

template <typename T>
Matrix4<T> Matrix4<T>::operator+(const Matrix4<T>& other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] + other.v[z];
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator-(const Matrix4<T>& other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] - other.v[z];
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator+(T other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] + other;
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator-(T other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] - other;
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator*(T other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] * other;
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator/(T other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] / other;
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator%(T other) const {
  Matrix4<T> res;
  for (size_t z = 0; z < 16; z++) {
    res.v[z] = this->v[z] % other;
  }
  return res;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator+=(const Matrix4<T>& other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] += other.v[z];
  }
  return *this;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator-=(const Matrix4<T>& other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] -= other.v[z];
  }
  return *this;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator+=(T other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] += other;
  }
  return *this;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator-=(T other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] -= other;
  }
  return *this;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator*=(T other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] *= other;
  }
  return *this;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator/=(T other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] /= other;
  }
  return *this;
}

template <typename T>
Matrix4<T>& Matrix4<T>::operator%=(T other) {
  for (size_t z = 0; z < 16; z++) {
    this->v[z] %= other;
  }
  return *this;
}

template <typename T>
Vector4<T> Matrix4<T>::operator*(const Vector4<T>& other) const {
  return Vector4<T>(
      this->m[0][0] * other.x + this->m[1][0] * other.y + this->m[2][0] * other.z + this->m[3][0] * other.w,
      this->m[0][1] * other.x + this->m[1][1] * other.y + this->m[2][1] * other.z + this->m[3][1] * other.w,
      this->m[0][2] * other.x + this->m[1][2] * other.y + this->m[2][2] * other.z + this->m[3][2] * other.w,
      this->m[0][3] * other.x + this->m[1][3] * other.y + this->m[2][3] * other.z + this->m[3][3] * other.w);
}

template <typename T>
Matrix4<T> Matrix4<T>::operator*(const Matrix4<T>& other) const {
  Matrix4<T> res;
  for (size_t x = 0; x < 4; x++) {
    for (size_t y = 0; y < 4; y++) {
      double value = 0;
      for (size_t z = 0; z < 4; z++) {
        value += this->m[z][y] * other.m[x][z];
      }
      res.m[x][y] = value;
    }
  }
  return res;
}

template <typename T>
Matrix4<T> Matrix4<T>::operator*=(const Matrix4<T>& other) {
  Matrix4<T> res = *this * other;
  *this = res;
  return *this;
}

template <typename T>
bool Matrix4<T>::operator==(const Matrix4<T>& other) const {
  for (size_t z = 0; z < 16; z++) {
    if (this->v[z] != other.v[z]) {
      return false;
    }
  }
  return true;
}

template <typename T>
bool Matrix4<T>::operator!=(const Matrix4<T>& other) const {
  return !(this->operator==(other));
}

template <typename T>
Matrix4<T> Matrix4<T>::transposition() const {
  Matrix4<T> res;
  for (size_t x = 0; x < 4; x++) {
    for (size_t y = 0; y < 4; y++) {
      res.m[y][x] = this->m[x][y];
    }
  }
  return res;
}

template <typename T>
Matrix4<T>& Matrix4<T>::transpose() {
  Matrix4<T> t = this->transposition();
  *this = t;
  return *this;
}

template <typename T>
Matrix4<T> Matrix4<T>::inverse() const {
  Matrix4<T> res = *this;
  res.invert();
  return res;
}

template <typename T>
Matrix4<T>& Matrix4<T>::invert() {
  Matrix4<T> left = *this;
  *this = Matrix4<T>(); // identity

  for (size_t z = 0; z < 4; z++) {
    // make m[z][z] be 1
    double row_divisor = left.m[z][z];
    if (row_divisor == 0.0) {
      throw std::runtime_error("matrix is not invertible");
    }
    for (size_t x = 0; x < 4; x++) {
      left.m[x][z] /= row_divisor;
      this->m[x][z] /= row_divisor;
    }

    // make m[z][y] be zero for all y != z by adding rows
    for (size_t y = 0; y < 4; y++) {
      if (y == z) {
        continue;
      }
      double add_factor = -left.m[z][y];
      if (add_factor == 0) {
        continue;
      }
      for (size_t x = 0; x < 4; x++) {
        left.m[x][y] += left.m[x][z] * add_factor;
        this->m[x][y] += this->m[x][z] * add_factor;
      }
    }
  }

  return *this;
}

template <typename T>
std::string Matrix4<T>::str() const {
  return string_printf("[[%g, %g, %g, %g], [%g, %g, %g, %g], [%g, %g, %g, %g], [%g, %g, %g, %g]]",
      this->m[0][0], this->m[1][0], this->m[2][0], this->m[3][0],
      this->m[0][1], this->m[1][1], this->m[2][1], this->m[3][1],
      this->m[0][2], this->m[1][2], this->m[2][2], this->m[3][2],
      this->m[0][3], this->m[1][3], this->m[2][3], this->m[3][3]);
}
