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
    parse_error(const std::string& what);
  };
  // Thrown when a JSONObject is accessed as the wrong type
  class type_error : public std::runtime_error {
  public:
    type_error(const std::string& what);
  };
  // Thrown when a key doesn't exist in a dictionary
  class key_error : public std::runtime_error {
  public:
    key_error(const std::string& what);
  };
  // Thrown when an element doesn't exist in a list
  class index_error : public std::runtime_error {
  public:
    index_error(const std::string& what);
  };

  using list_type = std::vector<std::shared_ptr<JSONObject>>;
  using dict_type = std::unordered_map<std::string, std::shared_ptr<JSONObject>>;

  static std::shared_ptr<JSONObject> parse(StringReader& r);
  static std::shared_ptr<JSONObject> parse(const char* s, size_t size);
  static std::shared_ptr<JSONObject> parse(const std::string& s);

  // Direct constructors. Use these when generating JSON to be sent/written/etc.
  JSONObject(); // null
  JSONObject(bool x); // true/false
  JSONObject(const char* x); // string
  JSONObject(const char* x, size_t size); // string
  JSONObject(const std::string& x); // string
  JSONObject(std::string&& x); // string
  JSONObject(int64_t x); // integer
  JSONObject(double x); // float
  JSONObject(const std::vector<JSONObject>& x); // list
  JSONObject(std::vector<JSONObject>&& x); // list
  JSONObject(const list_type& x); // list
  JSONObject(list_type&& x); // list
  JSONObject(const std::unordered_map<std::string, JSONObject>& x); // dict
  JSONObject(std::unordered_map<std::string, JSONObject>&& x); // dict
  JSONObject(const dict_type& x); // dict
  JSONObject(dict_type&& x); // dict

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

  // Exporters. serialize() returns the shortest possible text representation of
  // the object; format() returns a longer representation with whitespace
  // inserted to make it more human-readable.
  std::string serialize() const;
  std::string format(size_t indent_level = 0) const;

private:
  std::variant<
    const void*, // We use this type for JSON null (by storing nullptr here)
    bool,
    int64_t, // This is convertible to double implicitly in as_float()
    double, // This is convertible to int implicitly in as_int()
    std::string,
    list_type,
    dict_type
  > value;
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
std::shared_ptr<JSONObject> make_json_dict(
    std::initializer_list<std::pair<const char*, std::shared_ptr<JSONObject>>> values);

template <typename T>
std::shared_ptr<JSONObject> make_json_list(const std::vector<T>& values) {
  std::vector<std::shared_ptr<JSONObject>> vec;
  vec.reserve(values.size());
  for (const auto& it : values) {
    vec.emplace_back(new JSONObject(it));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(move(vec)));
}

template <typename T>
std::shared_ptr<JSONObject> make_json_list(std::initializer_list<T> values) {
  std::vector<std::shared_ptr<JSONObject>> vec;
  vec.reserve(values.size());
  for (const auto& it : values) {
    vec.emplace_back(new JSONObject(it));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(move(vec)));
}

template <typename V>
std::shared_ptr<JSONObject> make_json_dict(const std::unordered_map<std::string, V>& values) {
  std::unordered_map<std::string, std::shared_ptr<JSONObject>> dict;
  for (const auto& it : values) {
    dict.emplace(it.first, new JSONObject(it.second));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(move(dict)));
}

template <typename V>
std::shared_ptr<JSONObject> make_json_dict(std::initializer_list<std::pair<std::string, V>> values) {
  std::unordered_map<std::string, std::shared_ptr<JSONObject>> dict;
  for (const auto& it : values) {
    dict.emplace(it.first, new JSONObject(it.second));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(move(dict)));
}

template <typename V>
std::shared_ptr<JSONObject> make_json_dict(std::initializer_list<std::pair<const char*, V>> values) {
  std::unordered_map<std::string, std::shared_ptr<JSONObject>> dict;
  for (const auto& it : values) {
    dict.emplace(it.first, new JSONObject(it.second));
  }
  return std::shared_ptr<JSONObject>(new JSONObject(move(dict)));
}
