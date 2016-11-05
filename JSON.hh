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
  explicit JSONObject(bool x);
  explicit JSONObject(const char* s);
  JSONObject(const char* s, size_t size);
  explicit JSONObject(const std::string& x);
  JSONObject(std::string&& x);
  explicit JSONObject(int64_t x);
  explicit JSONObject(double x);
  explicit JSONObject(const std::vector<JSONObject>& x);
  JSONObject(std::vector<JSONObject>&& x);
  explicit JSONObject(const std::unordered_map<std::string, JSONObject>& x);
  JSONObject(std::unordered_map<std::string, JSONObject>&& x);

  JSONObject(const JSONObject& rhs);
  JSONObject(JSONObject&& rhs);
  JSONObject& operator=(const JSONObject& rhs);

  ~JSONObject();

  bool operator==(const JSONObject& other) const;
  bool operator!=(const JSONObject& other) const;

  JSONObject& operator[](const std::string& key);
  const JSONObject& operator[](const std::string& key) const;
  JSONObject& operator[](size_t index);
  const JSONObject& operator[](size_t index) const;

  std::unordered_map<std::string, JSONObject>& as_dict();
  const std::unordered_map<std::string, JSONObject>& as_dict() const;
  std::vector<JSONObject>& as_list();
  const std::vector<JSONObject>& as_list() const;
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
