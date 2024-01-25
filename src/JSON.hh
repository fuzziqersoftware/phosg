#pragma once

#include <compare>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Strings.hh"
#include "Types.hh"

class JSON {
public:
  enum class StringEscapeMode {
    STANDARD = 0,
    HEX,
    CONTROL_ONLY,
  };
  static std::string escape_string(const std::string& s, StringEscapeMode mode = StringEscapeMode::STANDARD);

  using list_type = std::vector<std::unique_ptr<JSON>>;
  using dict_type = std::unordered_map<std::string, std::unique_ptr<JSON>>;

private:
  template <typename T>
  inline static constexpr bool is_non_null_trivial_primitive_v = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool>;
  template <typename T>
  inline static constexpr bool is_primitive_v = std::is_same_v<T, std::nullptr_t> || std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool> || std::is_same_v<T, std::string>;
  template <typename T>
  inline static constexpr bool is_non_null_primitive_v = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool> || std::is_same_v<T, std::string>;

public:
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
    return JSON(std::vector<std::unique_ptr<JSON>>());
  }
  static inline JSON list(std::initializer_list<JSON> values) {
    list_type v;
    v.reserve(values.size());
    for (const auto& item : values) {
      v.emplace_back(new JSON(item));
    }
    return JSON(std::move(v));
  }
  static inline JSON dict() {
    return JSON(std::unordered_map<std::string, std::unique_ptr<JSON>>());
  }
  static inline JSON dict(std::initializer_list<std::pair<const std::string, JSON>> values) {
    dict_type v;
    for (const auto& item : values) {
      v.emplace(item.first, new JSON(item.second));
    }
    return JSON(std::move(v));
  }

  // Primitive type constructors
  JSON(); // null
  JSON(std::nullptr_t);
  JSON(bool x);
  JSON(const char* x);
  JSON(const char* x, size_t size);
  JSON(const std::string& x);
  JSON(std::string&& x);
  JSON(int64_t x);
  JSON(double x);

  template <typename T>
    requires(std::is_integral_v<T>)
  JSON(T x) : JSON(static_cast<int64_t>(x)) {}

  template <typename T>
    requires(std::is_enum_v<T>)
  JSON(T x) : JSON(name_for_enum<T>(x)) {}

  // List constructors
  template <typename T>
  JSON(const std::vector<T>& x) {
    list_type v;
    v.reserve(x.size());
    for (const auto& item : x) {
      v.emplace_back(new JSON(item));
    }
    this->value = std::move(v);
  }

  // Dict constructors
  JSON(std::initializer_list<std::pair<const char*, JSON>> values);
  JSON(std::initializer_list<std::pair<const std::string, JSON>> values);
  template <typename T>
    requires(is_primitive_v<T>)
  JSON(const std::unordered_map<std::string, T>& x) {
    dict_type v;
    for (const auto& item : x) {
      v.emplace(item.first, new JSON(item.second));
    }
    this->value = std::move(v);
  }

  // Copy/move constructors
  JSON(const JSON& rhs);
  JSON(JSON&& rhs) = default;
  JSON& operator=(const JSON& rhs);
  JSON& operator=(JSON&& rhs) = default;

  ~JSON() = default;

  enum SerializeOption : uint32_t {
    // These options enable serialization behaviors, some of which don't conform
    // to the JSON standard.

    // This option adds whitespace and line breaks to the output to make it
    // easier for humans to read. The output is still standard-compliant.
    FORMAT = 0x04,
    // If this is enabled, all integers are serialized in hexadecimal. This is
    // not standard-compliant, but JSON::parse can parse output generated with
    // this option if disable_extensions is false (the default).
    HEX_INTEGERS = 0x01,
    // If this is enabled, null, true, and false are serialized as single
    // characters (n, t, and f). This is not standard-compliant, but JSON::parse
    // can parse output generated with this option if disable_extensions is
    // false (the default).
    ONE_CHARACTER_TRIVIAL_CONSTANTS = 0x02,
    // If this is enabled, keys in dictionaries are sorted. If not enabled,
    // keys are serialized in the order they're stored, which is arbitrary.
    // Sorting takes a bit of extra time and memory, so if the resulting JSON
    // isn't expected to be read by a human, it's often not worth it. When this
    // is enabled, the output is still standard-compliant.
    SORT_DICT_KEYS = 0x08,
    // If this is enabled, non-ASCII bytes in strings are encoded in a shorter
    // form with the \x code rather than the \u code. This is not standard-
    // compliant.
    HEX_ESCAPE_CODES = 0x10,
    // If this is enabled, non-ASCII bytes in strings are not encoded at all;
    // only the double-quote (") character will be escaped. This is not
    // standard-compliant, but makes non-English UTF-8 strings easier to work
    // with manually.
    ESCAPE_CONTROLS_ONLY = 0x20,
  };
  std::string serialize(uint32_t options = 0, size_t indent_level = 0) const;

  // Comparison operators
  std::partial_ordering operator<=>(const JSON& other) const;
  std::partial_ordering operator<=>(std::nullptr_t) const; // Same as is_null()
  std::partial_ordering operator<=>(bool v) const;
  std::partial_ordering operator<=>(const char* v) const;
  std::partial_ordering operator<=>(const std::string& v) const;
  template <typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
  std::partial_ordering operator<=>(T v) const {
    const int64_t* stored_vi = std::get_if<2>(&this->value);
    if (stored_vi != nullptr) {
      return *stored_vi <=> v;
    }
    const auto* stored_vf = std::get_if<3>(&this->value);
    return (stored_vf == nullptr ? std::partial_ordering::unordered : (*stored_vf <=> v));
  }
  std::partial_ordering operator<=>(const list_type& v) const;
  std::partial_ordering operator<=>(const dict_type& v) const;

  template <typename T>
  bool operator==(T v) const {
    return (this->operator<=>(v) == std::partial_ordering::equivalent);
  }
  template <typename T>
  bool operator!=(T v) const {
    return (this->operator<=>(v) != std::partial_ordering::equivalent);
  }
  template <typename T>
  bool operator<(T v) const {
    return (this->operator<=>(v) == std::partial_ordering::less);
  }
  template <typename T>
  bool operator<=(T v) const {
    auto r = this->operator<=>(v);
    return (r == std::partial_ordering::equivalent) || (r == std::partial_ordering::less);
  }
  template <typename T>
  bool operator>(T v) const {
    return (this->operator<=>(v) == std::partial_ordering::greater);
  }
  template <typename T>
  bool operator>=(T v) const {
    auto r = this->operator<=>(v);
    return (r == std::partial_ordering::equivalent) || (r == std::partial_ordering::greater);
  }

  // Dict and list element accessors. These are just shorthand for e.g.
  // obj.as_list().at() / obj.as_dict.at()
  JSON& at(const std::string& key);
  const JSON& at(const std::string& key) const;
  JSON& at(size_t index);
  const JSON& at(size_t index) const;

  inline bool get_bool(const std::string& key) const {
    return this->at(key).as_bool();
  }
  inline bool get_bool(size_t index) const {
    return this->at(index).as_bool();
  }
  inline bool get_bool(const std::string& key, bool default_value) const {
    try {
      return this->at(key).as_bool();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  inline bool get_bool(size_t index, bool default_value) const {
    try {
      return this->at(index).as_bool();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }

  int64_t get_int(const std::string& key) const {
    return this->at(key).as_int();
  }
  int64_t get_int(size_t index) const {
    return this->at(index).as_int();
  }
  int64_t get_int(const std::string& key, int64_t default_value) const {
    try {
      return this->at(key).as_int();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  int64_t get_int(size_t index, int64_t default_value) const {
    try {
      return this->at(index).as_int();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }

  double get_float(const std::string& key) const {
    return this->at(key).as_float();
  }
  double get_float(size_t index) const {
    return this->at(index).as_float();
  }
  double get_float(const std::string& key, double default_value) const {
    try {
      return this->at(key).as_float();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  double get_float(size_t index, double default_value) const {
    try {
      return this->at(index).as_float();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }

  template <typename T>
    requires(std::is_enum_v<T>)
  T get_enum(const std::string& key) const {
    return enum_for_name<T>(this->at(key).as_string().c_str());
  }
  template <typename T>
    requires(std::is_enum_v<T>)
  T get_enum(size_t index) const {
    return enum_for_name<T>(this->at(index).as_string().c_str());
  }
  template <typename T>
    requires(std::is_enum_v<T>)
  T get_enum(const std::string& key, T default_value) const {
    try {
      return enum_for_name<T>(this->at(key).as_string().c_str());
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  template <typename T>
    requires(std::is_enum_v<T>)
  T get_enum(size_t index, T default_value) const {
    try {
      return enum_for_name<T>(this->at(index).as_string().c_str());
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }

  inline const std::string& get_string(const std::string& key) const {
    return this->at(key).as_string();
  }
  inline const std::string& get_string(size_t index) const {
    return this->at(index).as_string();
  }
  inline const std::string& get_string(const std::string& key, const std::string& default_value) const {
    try {
      return this->at(key).as_string();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  inline const std::string& get_string(size_t index, const std::string& default_value) const {
    try {
      return this->at(index).as_string();
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }
  inline const list_type& get_list(const std::string& key) const {
    return this->at(key).as_list();
  }
  inline const list_type& get_list(size_t index) const {
    return this->at(index).as_list();
  }
  inline const dict_type& get_dict(const std::string& key) const {
    return this->at(key).as_dict();
  }
  inline const dict_type& get_dict(size_t index) const {
    return this->at(index).as_dict();
  }
  inline const JSON& get(const std::string& key, const JSON& default_value) const {
    try {
      return this->at(key);
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

  // Type inspectors
  inline bool is_null() const {
    return holds_alternative<std::nullptr_t>(this->value);
  }
  inline bool is_bool() const {
    return holds_alternative<bool>(this->value);
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
  inline bool is_list() const {
    return holds_alternative<list_type>(this->value);
  }
  inline bool is_dict() const {
    return holds_alternative<dict_type>(this->value);
  }

  // Container-like functions. These throw type_error if the value is not a list
  // or dict; otherwise, they behave like the corresponding functions on
  // std::vector or std::unordered_map.
  size_t size() const;
  bool empty() const;
  void clear();

  inline void swap(JSON& other) {
    this->value.swap(other.value);
  }

  // Vector-like functions
  inline JSON& front() {
    return *this->as_list().front();
  }
  inline const JSON& front() const {
    return *this->as_list().front();
  }
  inline JSON& back() {
    return *this->as_list().back();
  }
  inline const JSON& back() const {
    return *this->as_list().back();
  }
  inline JSON& emplace_back(JSON&& item) {
    return *this->as_list().emplace_back(new JSON(std::move(item)));
  }
  inline void resize(size_t new_size, const JSON& new_values = nullptr) {
    auto& l = this->as_list();
    if (l.size() < new_size) {
      l.reserve(new_size);
      while (l.size() < new_size) {
        l.emplace_back(new JSON(new_values));
      }
    } else if (l.size() > new_size) {
      l.resize(new_size);
    }
  }

  // Map-like functions
  inline std::pair<dict_type::iterator, bool> emplace(std::string&& key, JSON&& item) {
    return this->as_dict().emplace(std::move(key), new JSON(std::move(item)));
  }
  inline std::pair<dict_type::iterator, bool> emplace(const std::string& key, JSON&& item) {
    return this->as_dict().emplace(key, new JSON(std::move(item)));
  }
  inline std::pair<dict_type::iterator, bool> insert(const std::string& key, const JSON& item) {
    return this->as_dict().emplace(key, new JSON(item));
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

  JSON(list_type&& x);
  JSON(dict_type&& x);

  std::variant<
      std::nullptr_t, // We use this type for JSON null
      bool,
      int64_t, // This is convertible to double implicitly in as_float()
      double, // This is convertible to int implicitly in as_int()
      std::string,
      list_type,
      dict_type>
      value;
};
