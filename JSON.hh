#pragma once

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>


class JSONObject {
public:

  // thrown when JSON can't be parsed
  class parse_error : public std::runtime_error {
  public:
    parse_error(const std::string& what);
  };

  // thrown when an object is accessed as the wrong type
  class type_error : public std::runtime_error {
  public:
    type_error(const std::string& what);
  };

  // thrown when a key doesn't exist in a dictionary
  class key_error : public std::runtime_error {
  public:
    key_error(const std::string& what);
  };

  // thrown when an element doesn't exist in a list
  class index_error : public std::runtime_error {
  public:
    index_error(const std::string& what);
  };

  class file_error : public std::runtime_error {
  public:
    file_error(const std::string& what);
  };

  enum Type {
    Null = 0,
    Bool,
    String,
    Integer,
    Float,
    List,
    Dict,
  };

  static JSONObject load(const std::string& filename);
  void save(const std::string& filename, bool format = false) const;

  static JSONObject parse(const std::string& s, size_t offset = 0);

  JSONObject();
  JSONObject(bool x);
  JSONObject(const std::string& x);
  JSONObject(int64_t x);
  JSONObject(double x);
  JSONObject(const std::vector<JSONObject>& x);
  JSONObject(const std::unordered_map<std::string, JSONObject>& x);

  JSONObject(const JSONObject& rhs);
  const JSONObject& operator=(const JSONObject& rhs);

  ~JSONObject();

  bool operator==(const JSONObject& other) const;
  bool operator!=(const JSONObject& other) const;

  JSONObject& operator[](const std::string& key);
  const JSONObject& operator[](const std::string& key) const;
  JSONObject& operator[](size_t index);
  const JSONObject& operator[](size_t index) const;

  const std::unordered_map<std::string, JSONObject>& as_dict() const;
  const std::vector<JSONObject>& as_list() const;
  int64_t as_int() const;
  double as_float() const;
  const std::string& as_string() const;
  bool as_bool() const;
  bool is_null() const;

  int source_length() const;

  std::string serialize() const;
  std::string format(size_t indent_level = 0) const;

private:
  Type type;
  int consumed_characters;

  // TODO: really there should be a subclass for each type, but this isn't used
  // in performance-critical code right now
  std::unordered_map<std::string, JSONObject> dict_data;
  std::vector<JSONObject> list_data;
  int64_t int_data;
  double float_data;
  std::string string_data;
  bool bool_data;
};
