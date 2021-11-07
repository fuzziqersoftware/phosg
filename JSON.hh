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

  // null
  JSONObject();

  // true/false
  JSONObject(bool x);

  // string
  JSONObject(const char* x);
  JSONObject(const char* x, size_t size);
  JSONObject(const std::string& x);
  JSONObject(std::string&& x);

  // integer
  JSONObject(int64_t x);

  // float
  JSONObject(double x);

  // list
  JSONObject(const std::vector<JSONObject>& x);
  JSONObject(std::vector<JSONObject>&& x);
  JSONObject(const list_type& x);
  JSONObject(list_type&& x);

  // dict
  JSONObject(const std::unordered_map<std::string, JSONObject>& x);
  JSONObject(std::unordered_map<std::string, JSONObject>&& x);
  JSONObject(const dict_type& x);
  JSONObject(dict_type&& x);

  // copy/move
  JSONObject(const JSONObject& rhs) = default;
  JSONObject(JSONObject&& rhs) = default;
  JSONObject& operator=(const JSONObject& rhs) = default;
  JSONObject& operator=(JSONObject&& rhs) = default;

  ~JSONObject() = default;

  bool operator==(const JSONObject& other) const;
  bool operator!=(const JSONObject& other) const;

  std::shared_ptr<JSONObject> at(const std::string& key);
  const std::shared_ptr<JSONObject> at(const std::string& key) const;
  std::shared_ptr<JSONObject> at(size_t index);
  const std::shared_ptr<JSONObject> at(size_t index) const;

  dict_type& as_dict();
  const dict_type& as_dict() const;
  list_type& as_list();
  const list_type& as_list() const;
  int64_t as_int() const;
  double as_float() const;
  std::string& as_string();
  const std::string& as_string() const;
  bool as_bool() const;

  bool is_dict() const;
  bool is_list() const;
  bool is_int() const;
  bool is_float() const;
  bool is_string() const;
  bool is_bool() const;
  bool is_null() const;

  int source_length() const;

  std::string serialize() const;
  std::string format(size_t indent_level = 0) const;

private:
  std::variant<
    const void*, // We use this for nulls (storing nullptr here)
    bool,
    int64_t, // This is convertible to double inplicitly in as_float()
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
