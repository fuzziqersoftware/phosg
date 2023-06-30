#pragma once

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Strings.hh"

std::string escape_json_string(const std::string& s);

class JSONObject {
public:
  // Thrown when JSON can't be parsed (note that std::out_of_range may also be
  // thrown if the JSON contains an unterminated string, list, or dict)
  class parse_error : public std::runtime_error {
  public:
    explicit parse_error(const std::string& what);
  };
  // Thrown when a JSONObject is accessed as the wrong type
  class type_error : public std::runtime_error {
  public:
    explicit type_error(const std::string& what);
  };
  // Thrown when a key doesn't exist in a dictionary
  class key_error : public std::runtime_error {
  public:
    explicit key_error(const std::string& what);
  };
  // Thrown when an element doesn't exist in a list
  class index_error : public std::runtime_error {
  public:
    explicit index_error(const std::string& what);
  };

  using list_type = std::vector<std::shared_ptr<JSONObject>>;
  using dict_type = std::unordered_map<std::string, std::shared_ptr<JSONObject>>;

  // JSON text parsers. If disable_extensions is true, the following
  // nonstandard extensions are not allowed:
  // - Trailing commas in lists and dictionaries
  // - Hexadecimal integers
  // - Comments
  // These extensions do not make any standard-compliant JSON unparseable, so
  // they are enabled by default. The inverse function, serialize, has the
  // opposite behavior - it always generates standard-compilant JSON unless
  // extensions are specifically requested.
  // The StringReader variant of this function does not throw if there's extra
  // data after a valid JSON object; the other variants do (unless it's only
  // whitespace).
  static std::shared_ptr<JSONObject> parse(
      StringReader& r, bool disable_extensions = false);
  static std::shared_ptr<JSONObject> parse(
      const char* s, size_t size, bool disable_extensions = false);
  static std::shared_ptr<JSONObject> parse(
      const std::string& s, bool disable_extensions = false);

  // Direct constructors. Use these when generating JSON to be sent/written/etc.
  JSONObject(); // null
  explicit JSONObject(bool x); // true/false
  explicit JSONObject(const char* x); // string
  JSONObject(const char* x, size_t size); // string
  explicit JSONObject(const std::string& x); // string
  explicit JSONObject(std::string&& x); // string
  explicit JSONObject(int64_t x); // integer
  explicit JSONObject(double x); // float
  explicit JSONObject(const std::vector<JSONObject>& x); // list
  explicit JSONObject(std::vector<JSONObject>&& x); // list
  explicit JSONObject(const list_type& x); // list
  explicit JSONObject(list_type&& x); // list
  explicit JSONObject(const std::unordered_map<std::string, JSONObject>& x); // dict
  explicit JSONObject(std::unordered_map<std::string, JSONObject>&& x); // dict
  explicit JSONObject(const dict_type& x); // dict
  explicit JSONObject(dict_type&& x); // dict

  // Copy/move constructors
  JSONObject(const JSONObject& rhs) = default;
  JSONObject(JSONObject&& rhs) = default;
  JSONObject& operator=(const JSONObject& rhs) = default;
  JSONObject& operator=(JSONObject&& rhs) = default;

  ~JSONObject() = default;

  bool operator==(const JSONObject& other) const;
  bool operator!=(const JSONObject& other) const;

  // Dict and list element accessors. These are just shorthand for e.g.
  // obj.as_list().at() / obj.as_dict.at()
  std::shared_ptr<JSONObject> at(const std::string& key);
  const std::shared_ptr<JSONObject> at(const std::string& key) const;
  std::shared_ptr<JSONObject> at(size_t index);
  const std::shared_ptr<JSONObject> at(size_t index) const;

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
  bool is_dict() const;
  bool is_list() const;
  bool is_int() const;
  bool is_float() const;
  bool is_string() const;
  bool is_bool() const;
  bool is_null() const;

  enum SerializeOption : uint32_t {
    // These options enable serialization behaviors, most of which don't
    // conform to the JSON standard.

    // This option adds whitespace to the output to make it easier for humans
    // to read. The output is still standard-compliant.
    FORMAT = 4,
    // If this is enabled, all integers are serialized in hexadecimal. This is
    // not standard-compliant, but JSONObject can parse output generated with
    // this option if disable_extensions is false (the default).
    HEX_INTEGERS = 1,
    // If this is enabled, null, true, and false are serialized as single
    // characters (n, t, and f). This is not standard-compliant, but JSONObject
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

private:
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

std::shared_ptr<JSONObject> make_json_null();
std::shared_ptr<JSONObject> make_json_bool(bool x);
std::shared_ptr<JSONObject> make_json_num(double x);
std::shared_ptr<JSONObject> make_json_int(int64_t x);
std::shared_ptr<JSONObject> make_json_str(const char* s);
std::shared_ptr<JSONObject> make_json_str(const std::string& s);
std::shared_ptr<JSONObject> make_json_str(std::string&& s);
std::shared_ptr<JSONObject> make_json_list(JSONObject::list_type&& values);
std::shared_ptr<JSONObject> make_json_list(
    std::initializer_list<std::shared_ptr<JSONObject>> values);
std::shared_ptr<JSONObject> make_json_dict(JSONObject::dict_type&& values);
std::shared_ptr<JSONObject> make_json_dict(
    std::initializer_list<std::pair<const char*, std::shared_ptr<JSONObject>>> values);

template <typename T>
std::shared_ptr<JSONObject> make_json_list(const std::vector<T>& values) {
  std::vector<std::shared_ptr<JSONObject>> vec;
  vec.reserve(values.size());
  for (const auto& it : values) {
    vec.emplace_back(new JSONObject(it));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(std::move(vec)));
}

template <typename T>
std::shared_ptr<JSONObject> make_json_list(std::initializer_list<T> values) {
  std::vector<std::shared_ptr<JSONObject>> vec;
  vec.reserve(values.size());
  for (const auto& it : values) {
    vec.emplace_back(new JSONObject(it));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(std::move(vec)));
}

template <typename V>
std::shared_ptr<JSONObject> make_json_dict(const std::unordered_map<std::string, V>& values) {
  std::unordered_map<std::string, std::shared_ptr<JSONObject>> dict;
  for (const auto& it : values) {
    dict.emplace(it.first, new JSONObject(it.second));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(std::move(dict)));
}

template <typename V>
std::shared_ptr<JSONObject> make_json_dict(std::initializer_list<std::pair<std::string, V>> values) {
  std::unordered_map<std::string, std::shared_ptr<JSONObject>> dict;
  for (const auto& it : values) {
    dict.emplace(it.first, new JSONObject(it.second));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(std::move(dict)));
}

template <typename V>
std::shared_ptr<JSONObject> make_json_dict(std::initializer_list<std::pair<const char*, V>> values) {
  std::unordered_map<std::string, std::shared_ptr<JSONObject>> dict;
  for (const auto& it : values) {
    dict.emplace(it.first, new JSONObject(it.second));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(std::move(dict)));
}
