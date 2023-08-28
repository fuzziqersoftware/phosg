#pragma once

#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Strings.hh"
#include "Types.hh"

std::string escape_json_string(const std::string& s);

class JSON {
public:
  template <typename T>
  inline static constexpr bool is_non_null_trivial_primitive_v = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool>;
  template <typename T>
  inline static constexpr bool is_non_null_primitive_v = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool> || std::is_same_v<T, std::string>;

  // Thrown when JSON can't be parsed (note that std::out_of_range may also be
  // thrown if the JSON contains an unterminated string, list, or dict)
  class parse_error : public std::runtime_error {
  public:
    explicit parse_error(const std::string& what);
  };
  // Thrown when a JSON is accessed as the wrong type
  class type_error : public std::runtime_error {
  public:
    explicit type_error(const std::string& what);
  };

  using list_type = std::vector<JSON>;
  using dict_type = std::unordered_map<std::string, JSON>;

  // JSON text parsers. If disable_extensions is true, the following
  // nonstandard extensions are not allowed:
  // - Trailing commas in lists and dictionaries
  // - Hexadecimal integers
  // - Single-character trivial constants (n/t/f for null/true/false)
  // - Comments
  // These extensions do not make any standard-compliant JSON unparseable, so
  // they are enabled by default. The inverse function, serialize, has the
  // opposite behavior - it always generates standard-compilant JSON unless
  // extensions are specifically requested.
  // The StringReader variant of this function does not throw if there's extra
  // data after a valid JSON object; the other variants do (unless it's only
  // whitespace).
  static JSON parse(StringReader& r, bool disable_extensions = false);
  static JSON parse(const char* s, size_t size, bool disable_extensions = false);
  static JSON parse(const std::string& s, bool disable_extensions = false);

  // Because the statement `JSON v = {};` is ambiguous, these functions
  // exist to explicitly construct an empty list or dictionary.
  static inline JSON list() {
    return JSON(std::vector<JSON>());
  }
  static inline JSON list(std::initializer_list<JSON> values) {
    return JSON(std::vector<JSON>(values));
  }
  static inline JSON dict() {
    return JSON(std::unordered_map<std::string, JSON>());
  }
  static inline JSON dict(std::initializer_list<dict_type::value_type> values) {
    return JSON(std::unordered_map<std::string, JSON>(values));
  }

  // Primitive type constructors
  JSON(); // null
  JSON(nullptr_t);
  JSON(bool x);
  JSON(const char* x);
  JSON(const char* x, size_t size);
  JSON(const std::string& x);
  JSON(std::string&& x);
  JSON(int64_t x);
  JSON(double x);

  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  JSON(T x) : JSON(static_cast<int64_t>(x)) {}

  // List constructors
  JSON(const list_type& x);
  JSON(list_type&& x);
  template <typename T>
  JSON(const std::vector<T>& x) {
    list_type v;
    v.reserve(x.size());
    for (const auto& item : x) {
      v.emplace_back(item);
    }
    this->value = v;
  }

  // Dict constructors
  JSON(const std::unordered_map<std::string, JSON>& x);
  JSON(std::unordered_map<std::string, JSON>&& x);
  JSON(std::initializer_list<std::pair<const char*, JSON>> values);
  JSON(std::initializer_list<std::pair<const std::string, JSON>> values);
  template <typename T>
  JSON(const std::unordered_map<std::string, T>& x) {
    dict_type v;
    for (const auto& item : x) {
      v.emplace(item.first, item.second);
    }
    this->value = v;
  }

  // Copy/move constructors
  JSON(const JSON& rhs) = default;
  JSON(JSON&& rhs) = default;
  JSON& operator=(const JSON& rhs) = default;
  JSON& operator=(JSON&& rhs) = default;

  ~JSON() = default;

  enum SerializeOption : uint32_t {
    // These options enable serialization behaviors, some of which don't conform
    // to the JSON standard.

    // This option adds whitespace and line breaks to the output to make it
    // easier for humans to read. The output is still standard-compliant.
    FORMAT = 4,
    // If this is enabled, all integers are serialized in hexadecimal. This is
    // not standard-compliant, but JSON::parse can parse output generated with
    // this option if disable_extensions is false (the default).
    HEX_INTEGERS = 1,
    // If this is enabled, null, true, and false are serialized as single
    // characters (n, t, and f). This is not standard-compliant, but JSON::parse
    // can parse output generated with this option if disable_extensions is
    // false (the default).
    ONE_CHARACTER_TRIVIAL_CONSTANTS = 2,
    // If this is enabled, keys in dictionaries are sorted. If not enabled,
    // keys are serialized in the order they're stored, which is arbitrary.
    // Sorting takes a bit of extra time and memory, so if the resulting JSON
    // isn't expected to be read by a human, it's often not worth it. When this
    // is enabled, the output is still standard-compliant.
    SORT_DICT_KEYS = 8,
  };
  std::string serialize(uint32_t options = 0, size_t indent_level = 0) const;

  bool operator==(const JSON& other) const;
  bool operator!=(const JSON& other) const;
  bool operator<(const JSON& other) const;
  bool operator<=(const JSON& other) const;
  bool operator>(const JSON& other) const;
  bool operator>=(const JSON& other) const;

  bool is_orderable_with(const JSON& other) const;

  // Shortcut comparison operators for common types. These return false if the
  // stored value is the wrong type, with the exception that ints and doubles
  // may be compared as expected.
  inline bool operator==(nullptr_t) const {
    return std::holds_alternative<nullptr_t>(this->value);
  }

  inline bool operator==(bool v) const {
    const auto* stored_v = std::get_if<bool>(&this->value);
    return (stored_v != nullptr) && (*stored_v == v);
  }

  inline bool operator==(const char* v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v == v);
  }
  inline bool operator<(const char* v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v < v);
  }
  inline bool operator<=(const char* v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v <= v);
  }
  inline bool operator>(const char* v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v > v);
  }
  inline bool operator>=(const char* v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v >= v);
  }

  inline bool operator==(const std::string& v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v == v);
  }
  inline bool operator<(const std::string& v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v < v);
  }
  inline bool operator<=(const std::string& v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v <= v);
  }
  inline bool operator>(const std::string& v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v > v);
  }
  inline bool operator>=(const std::string& v) const {
    const auto* stored_v = std::get_if<std::string>(&this->value);
    return (stored_v != nullptr) && (*stored_v >= v);
  }

  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  inline bool operator==(T v) const {
    const auto* stored_v = std::get_if<int64_t>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v == v;
    }
    const auto* stored_v2 = std::get_if<double>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 == v);
  }
  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  inline bool operator<(T v) const {
    const auto* stored_v = std::get_if<int64_t>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v < v;
    }
    const auto* stored_v2 = std::get_if<double>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 < v);
  }
  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  inline bool operator<=(T v) const {
    const auto* stored_v = std::get_if<int64_t>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v <= v;
    }
    const auto* stored_v2 = std::get_if<double>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 <= v);
  }
  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  inline bool operator>(T v) const {
    const auto* stored_v = std::get_if<int64_t>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v > v;
    }
    const auto* stored_v2 = std::get_if<double>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 > v);
  }
  template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
  inline bool operator>=(T v) const {
    const auto* stored_v = std::get_if<int64_t>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v >= v;
    }
    const auto* stored_v2 = std::get_if<double>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 >= v);
  }

  inline bool operator==(double v) const {
    const auto* stored_v = std::get_if<double>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v == v;
    }
    const auto* stored_v2 = std::get_if<int64_t>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 == v);
  }
  inline bool operator<(double v) const {
    const auto* stored_v = std::get_if<double>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v < v;
    }
    const auto* stored_v2 = std::get_if<int64_t>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 < v);
  }
  inline bool operator<=(double v) const {
    const auto* stored_v = std::get_if<double>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v <= v;
    }
    const auto* stored_v2 = std::get_if<int64_t>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 <= v);
  }
  inline bool operator>(double v) const {
    const auto* stored_v = std::get_if<double>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v > v;
    }
    const auto* stored_v2 = std::get_if<int64_t>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 > v);
  }
  inline bool operator>=(double v) const {
    const auto* stored_v = std::get_if<double>(&this->value);
    if (stored_v != nullptr) {
      return *stored_v >= v;
    }
    const auto* stored_v2 = std::get_if<int64_t>(&this->value);
    return (stored_v2 != nullptr) && (*stored_v2 >= v);
  }

  inline bool operator==(const list_type& v) const {
    const auto* stored_v = std::get_if<list_type>(&this->value);
    return (stored_v != nullptr) && (*stored_v == v);
  }
  inline bool operator<(const list_type& v) const {
    const auto* stored_v = std::get_if<list_type>(&this->value);
    return (stored_v != nullptr) && (*stored_v < v);
  }
  inline bool operator<=(const list_type& v) const {
    const auto* stored_v = std::get_if<list_type>(&this->value);
    return (stored_v != nullptr) && (*stored_v <= v);
  }
  inline bool operator>(const list_type& v) const {
    const auto* stored_v = std::get_if<list_type>(&this->value);
    return (stored_v != nullptr) && (*stored_v > v);
  }
  inline bool operator>=(const list_type& v) const {
    const auto* stored_v = std::get_if<list_type>(&this->value);
    return (stored_v != nullptr) && (*stored_v >= v);
  }

  inline bool operator==(const dict_type& v) const {
    const auto* stored_v = std::get_if<dict_type>(&this->value);
    return (stored_v != nullptr) && (*stored_v == v);
  }

  template <typename T>
  bool operator!=(const T& v) const {
    return !this->operator==(v);
  }

  // Dict and list element accessors. These are just shorthand for e.g.
  // obj.as_list().at() / obj.as_dict.at()
  JSON& at(const std::string& key);
  const JSON& at(const std::string& key) const;
  JSON& at(size_t index);
  const JSON& at(size_t index) const;

  template <
      typename T,
      typename = std::enable_if_t<is_non_null_trivial_primitive_v<T>>>
  T get(const std::string& key, T default_value) const {
    try {
      return this->at(key);
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  template <
      typename T,
      typename = std::enable_if_t<std::is_enum_v<T>>>
  T get_enum(const std::string& key, T default_value) const {
    try {
      return enum_for_name<T>(this->at(key).as_string().c_str());
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  inline const std::string& get(const std::string& key, const std::string& default_value) const {
    try {
      return this->at(key).as_string();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  inline const list_type& get(const std::string& key, const list_type& default_value) const {
    try {
      return this->at(key).as_list();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  inline const dict_type& get(const std::string& key, const dict_type& default_value) const {
    try {
      return this->at(key).as_dict();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }

  // Type-checking accessors. If the object is the requested type, returns the
  // value; otherwise, throws type_error.
  // The int and float types are "special" in that they are implicitly
  // convertible to each other; for example, calling as_float() on an int object
  // does not throw and instead returns a (possibly less precise) value. To know
  // if an implicit conversion occurred, the caller can use is_int()/is_float().
  // This is designed this way because JSON does not differentiate between
  // integers and floating-point numbers; the separate types are an extension
  // implemented in this library in order to support precise values for the full
  // range of 64-bit integers.
  bool as_bool() const;
  int64_t as_int() const;
  double as_float() const;
  std::string& as_string();
  const std::string& as_string() const;
  list_type& as_list();
  const list_type& as_list() const;
  dict_type& as_dict();
  const dict_type& as_dict() const;

  // Type-checking implicit converters. These are just aliases for as_bool, etc.
  inline operator bool() const {
    return this->as_bool();
  }
  template <
      typename T,
      typename = std::enable_if_t<std::is_integral_v<T>>>
  operator T() const {
    return this->as_int();
  }
  inline operator double() const {
    return this->as_float();
  }
  inline operator std::string&() {
    return this->as_string();
  }
  inline operator const std::string&() const {
    return this->as_string();
  }
  inline operator list_type&() {
    return this->as_list();
  }
  inline operator const list_type&() const {
    return this->as_list();
  }
  inline operator dict_type&() {
    return this->as_dict();
  }
  inline operator const dict_type&() const {
    return this->as_dict();
  }

  // Type inspectors
  inline bool is_dict() const {
    return holds_alternative<dict_type>(this->value);
  }
  inline bool is_list() const {
    return holds_alternative<list_type>(this->value);
  }
  inline bool is_int() const {
    return holds_alternative<int64_t>(this->value);
  }
  inline bool is_float() const {
    return holds_alternative<double>(this->value);
  }
  inline bool is_string() const {
    return holds_alternative<std::string>(this->value);
  }
  inline bool is_bool() const {
    return holds_alternative<bool>(this->value);
  }
  inline bool is_null() const {
    return holds_alternative<nullptr_t>(this->value);
  }

  // Container-like functions. These throw type_error if the value is not a list
  // or dict; otherwise, they behave like the corresponding functions on
  // std::vector or std::unordered_map.
  size_t size() const;
  void clear();

  inline void swap(JSON& other) {
    this->value.swap(other.value);
  }

  // Vector-like functions
  inline JSON& front() {
    return this->as_list().front();
  }
  inline const JSON& front() const {
    return this->as_list().front();
  }
  inline JSON& back() {
    return this->as_list().back();
  }
  inline const JSON& back() const {
    return this->as_list().back();
  }
  inline JSON& emplace_back(JSON&& item) {
    return this->as_list().emplace_back(std::move(item));
  }
  inline void resize(size_t new_size, const JSON& new_values = nullptr) {
    this->as_list().resize(new_size, new_values);
  }

  // Map-like functions
  inline std::pair<dict_type::iterator, bool> emplace(std::string&& key, JSON&& item) {
    return this->as_dict().emplace(std::move(key), std::move(item));
  }
  inline std::pair<dict_type::iterator, bool> emplace(const std::string& key, JSON&& item) {
    return this->as_dict().emplace(key, std::move(item));
  }
  inline std::pair<dict_type::iterator, bool> insert(const std::string& key, const JSON& item) {
    return this->as_dict().emplace(key, item);
  }
  inline size_t erase(const std::string& key) {
    return this->as_dict().erase(key);
  }
  inline size_t count(const std::string& key) const {
    return this->as_dict().count(key);
  }
  inline bool contains(const std::string& key) const {
    return this->as_dict().contains(key);
  }

private:
  template <typename T>
  bool is() const {
    return holds_alternative<T>(this->value);
  }

  std::variant<
      nullptr_t, // We use this type for JSON null
      bool,
      int64_t, // This is convertible to double implicitly in as_float()
      double, // This is convertible to int implicitly in as_int()
      std::string,
      list_type,
      dict_type>
      value;
};
