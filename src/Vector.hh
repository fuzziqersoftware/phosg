#pragma once

#include <string>

template <typename T>
struct Vector2 {
  union {
    T x;
    T a;
  };
  union {
    T y;
    T b;
  };

  Vector2();
  Vector2(T x, T y);

  Vector2<T> operator-() const;

  Vector2<T> operator+(const Vector2<T>& other) const;
  Vector2<T> operator-(const Vector2<T>& other) const;
  Vector2<T> operator+(T other) const;
  Vector2<T> operator-(T other) const;
  Vector2<T> operator*(T other) const;
  Vector2<T> operator/(T other) const;
  Vector2<T> operator%(T other) const;
  Vector2<T>& operator+=(const Vector2<T>& other);
  Vector2<T>& operator-=(const Vector2<T>& other);
  Vector2<T>& operator+=(T other);
  Vector2<T>& operator-=(T other);
  Vector2<T>& operator*=(T other);
  Vector2<T>& operator/=(T other);
  Vector2<T>& operator%=(T other);

  bool operator!() const;
  bool operator==(const Vector2<T>& other) const;
  bool operator!=(const Vector2<T>& other) const;

  // for usage in set/map
  bool operator<(const Vector2<T>& other) const;

  T at(size_t dim) const;

  T norm1() const;
  double norm() const;
  T norm2() const;
  T dot(const Vector2<T>& other) const;

  std::string str() const;

  static constexpr size_t dimensions();
};

template <typename T>
struct Vector3 {
  union {
    T x;
    T rx;
    T r;
  };
  union {
    T y;
    T ry;
    T g;
  };
  union {
    T z;
    T rz;
    T b;
  };

  Vector3();
  Vector3(T x, T y, T z);
  Vector3(const Vector2<T>& xy, T z);

  Vector3<T> operator-() const;

  Vector3<T> operator+(const Vector3<T>& other) const;
  Vector3<T> operator-(const Vector3<T>& other) const;
  Vector3<T> operator+(T other) const;
  Vector3<T> operator-(T other) const;
  Vector3<T> operator*(T other) const;
  Vector3<T> operator/(T other) const;
  Vector3<T> operator%(T other) const;
  Vector3<T>& operator+=(const Vector3<T>& other);
  Vector3<T>& operator-=(const Vector3<T>& other);
  Vector3<T>& operator+=(T other);
  Vector3<T>& operator-=(T other);
  Vector3<T>& operator*=(T other);
  Vector3<T>& operator/=(T other);
  Vector3<T>& operator%=(T other);

  bool operator!() const;
  bool operator==(const Vector3<T>& other) const;
  bool operator!=(const Vector3<T>& other) const;

  // for usage in set/map
  bool operator<(const Vector3<T>& other) const;

  T at(size_t dim) const;

  T norm1() const;
  double norm() const;
  T norm2() const;
  T dot(const Vector3<T>& other) const;
  Vector3<T> cross(const Vector3<T>& other) const;

  std::string str() const;

  static constexpr size_t dimensions();
};

template <typename T>
struct Vector4 {
  union {
    T x;
    T r;
  };
  union {
    T y;
    T g;
  };
  union {
    T z;
    T b;
  };
  union {
    T w;
    T a;
  };

  Vector4();
  Vector4(T x, T y, T z, T w);
  Vector4(const Vector2<T>& xy, T z, T w);
  Vector4(const Vector3<T>& xyz, T w);

  Vector4<T> operator-() const;

  Vector4<T> operator+(const Vector4<T>& other) const;
  Vector4<T> operator-(const Vector4<T>& other) const;
  Vector4<T> operator+(T other) const;
  Vector4<T> operator-(T other) const;
  Vector4<T> operator*(T other) const;
  Vector4<T> operator/(T other) const;
  Vector4<T> operator%(T other) const;
  Vector4<T>& operator+=(const Vector4<T>& other);
  Vector4<T>& operator-=(const Vector4<T>& other);
  Vector4<T>& operator+=(T other);
  Vector4<T>& operator-=(T other);
  Vector4<T>& operator*=(T other);
  Vector4<T>& operator/=(T other);
  Vector4<T>& operator%=(T other);

  bool operator!() const;
  bool operator==(const Vector4<T>& other) const;
  bool operator!=(const Vector4<T>& other) const;

  // for usage in set/map
  bool operator<(const Vector4<T>& other) const;

  T at(size_t dim) const;

  T norm1() const;
  double norm() const;
  T norm2() const;
  T dot(const Vector4<T>& other) const;

  std::string str() const;

  static constexpr size_t dimensions();
};

template <typename T>
struct Matrix4 {
  union {
    T m[4][4];
    T v[16];
  };

  Matrix4();

  Matrix4<T> operator+(const Matrix4<T>& other) const;
  Matrix4<T> operator-(const Matrix4<T>& other) const;
  Matrix4<T> operator+(T other) const;
  Matrix4<T> operator-(T other) const;
  Matrix4<T> operator*(T other) const;
  Matrix4<T> operator/(T other) const;
  Matrix4<T> operator%(T other) const;
  Matrix4<T>& operator+=(const Matrix4<T>& other);
  Matrix4<T>& operator-=(const Matrix4<T>& other);
  Matrix4<T>& operator+=(T other);
  Matrix4<T>& operator-=(T other);
  Matrix4<T>& operator*=(T other);
  Matrix4<T>& operator/=(T other);
  Matrix4<T>& operator%=(T other);

  Vector4<T> operator*(const Vector4<T>& other) const;
  Matrix4<T> operator*(const Matrix4<T>& other) const;
  Matrix4<T> operator*=(const Matrix4<T>& other);

  bool operator==(const Matrix4<T>& other) const;
  bool operator!=(const Matrix4<T>& other) const;

  Matrix4<T> transposition() const;
  Matrix4<T>& transpose();

  Matrix4<T> inverse() const;
  Matrix4<T>& invert();

  std::string str() const;
};

#include "Vector-inl.hh"
