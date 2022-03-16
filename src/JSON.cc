#include "JSON.hh"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;


JSONObject::parse_error::parse_error(const string& what) : runtime_error(what) { }
JSONObject::type_error::type_error(const string& what) : runtime_error(what) { }
JSONObject::key_error::key_error(const string& what) : runtime_error(what) { }
JSONObject::index_error::index_error(const string& what) : runtime_error(what) { }

static void skip_whitespace_and_comments(StringReader& r) {
  bool reading_comment = false;
  while (!r.eof()) {
    char ch = r.get_s8(false);
    if (reading_comment) {
      if ((ch == '\n') || (ch == '\r')) {
        reading_comment = false;
      }
    } else {
      if ((ch == '/') && (r.pget_s8(r.where() + 1) == '/')) {
        reading_comment = true;
      } else if ((ch != ' ') && (ch != '\t') && (ch != '\r') && (ch != '\n')) {
        return;
      }
    }
    r.get_s8();
  }
}

shared_ptr<JSONObject> JSONObject::parse(StringReader& r) {
  skip_whitespace_and_comments(r);

  shared_ptr<JSONObject> ret(new JSONObject());
  char root_type_ch = r.get_s8(false);
  if (root_type_ch == '{') {

    dict_type data;
    char expected_separator = '{';
    char separator = r.get_s8();
    while (separator != '}') {
      if (separator != expected_separator) {
        throw parse_error("string is not a dictionary; pos=" + to_string(r.where()));
      }
      expected_separator = ',';

      skip_whitespace_and_comments(r);
      if (r.get_s8(false) == '}') {
        r.get_s8();
        break;
      }

      shared_ptr<JSONObject> key = JSONObject::parse(r);
      skip_whitespace_and_comments(r);

      if (r.get_s8() != ':') {
        throw parse_error("dictionary does not contain key/value pairs; pos=" + to_string(r.where()));
      }
      skip_whitespace_and_comments(r);

      data.emplace(key->as_string(), JSONObject::parse(r));
      skip_whitespace_and_comments(r);
      separator = r.get_s8();
    }

    ret->value = move(data);

  } else if (root_type_ch == '[') {
    list_type data;
    char expected_separator = '[';
    char separator = r.get_s8();
    while (separator != ']') {
      if (separator != expected_separator) {
        throw parse_error("string is not a list; pos=" + to_string(r.where()));
      }
      expected_separator = ',';

      skip_whitespace_and_comments(r);
      if (r.get_s8(false) == ']') {
        r.get_s8();
        break;
      }

      data.emplace_back(JSONObject::parse(r));
      skip_whitespace_and_comments(r);
      separator = r.get_s8();
    }

    ret->value = move(data);

  } else if (root_type_ch == '-' || root_type_ch == '+' || isdigit(root_type_ch)) {
    int64_t int_data;
    double float_data;
    bool is_int = true;

    bool negative = false;
    if (root_type_ch == '-') {
      negative = true;
      r.get_s8();
    }

    if (((r.where() + 2) < r.size()) &&
        (r.get_s8(false) == '0') &&
        (r.pget_s8(r.where() + 1) == 'x')) { // hex
      r.go(r.where() + 2);

      int_data = 0;
      while (!r.eof() && isxdigit(r.get_s8(false))) {
        int_data = (int_data << 4) | value_for_hex_char(r.get_s8());
      }

    } else { // decimal
      int_data = 0;
      while (!r.eof() && isdigit(r.get_s8(false))) {
        int_data = int_data * 10 + (r.get_s8() - '0');
      }

      double this_place = 0.1;
      float_data = int_data;
      if (!r.eof() && r.get_s8(false) == '.') {
        is_int = false;
        r.get_s8();
        while (!r.eof() && isdigit(r.get_s8(false))) {
          float_data += (r.get_s8() - '0') * this_place;
          this_place *= 0.1;
        }
      }

      char exp_specifier = r.eof() ? '\0' : r.get_s8(false);
      if (exp_specifier == 'e' || exp_specifier == 'E') {
        r.get_s8();
        char sign_char = r.get_s8(false);
        bool e_negative = sign_char == '-';
        if (sign_char == '-' || sign_char == '+') {
          r.get_s8();
        }

        int e = 0;
        while (!r.eof() && isdigit(r.get_s8(false))) {
          e = e * 10 + (r.get_s8() - '0');
        }

        if (e_negative) {
          for (; e > 0; e--) {
            int_data *= 0.1;
            float_data *= 0.1;
          }
        } else {
          for (; e > 0; e--) {
            int_data *= 10;
            float_data *= 10;
          }
        }
      }
    }

    if (negative) {
      int_data = -int_data;
      float_data = -float_data;
    }

    if (is_int) {
      ret->value = int_data;
    } else {
      ret->value = float_data;
    }

  } else if (root_type_ch == '\"') {
    r.get_s8();

    string data;
    while (r.get_s8(false) != '\"') {
      char ch = r.get_s8();
      if (ch == '\\') {
        ch = r.get_s8();
        if (ch == '\"') {
          data.push_back('\"');
        } else if (ch == '\\') {
          data.push_back('\\');
        } else if (ch == '/') {
          data.push_back('/');
        } else if (ch == 'b') {
          data.push_back('\b');
        } else if (ch == 'f') {
          data.push_back('\f');
        } else if (ch == 'n') {
          data.push_back('\n');
        } else if (ch == 'r') {
          data.push_back('\r');
        } else if (ch == 't') {
          data.push_back('\t');
        } else if (ch == 'u') {
          uint16_t value;
          try {
            value = value_for_hex_char(r.get_s8()) << 12;
            value |= value_for_hex_char(r.get_s8()) << 8;
            value |= value_for_hex_char(r.get_s8()) << 4;
            value |= value_for_hex_char(r.get_s8());
          } catch (const out_of_range&) {
            throw parse_error("incomplete unicode escape sequence in string; pos=" + to_string(r.where()));
          }
          // TODO: we should eventually be able to support this
          if (value & 0xFF00) {
            throw parse_error("non-ascii unicode character sequence in string; pos=" + to_string(r.where()));
          }
        } else {
          throw parse_error("invalid escape sequence in string; pos=" + to_string(r.where()));
        }
      } else { // not an escap sequence
        data.push_back(ch);
      }
    }
    r.get_s8();

    ret->value = move(data);

  } else if (!strncmp(r.peek(4), "null", 4)) {
    ret->value = nullptr;
    r.go(r.where() + 4);

  } else if (!strncmp(r.peek(4), "true", 4)) {
    ret->value = true;
    r.go(r.where() + 4);

  } else if (!strncmp(r.peek(5), "false", 5)) {
    ret->value = false;
    r.go(r.where() + 5);

  } else {
    throw parse_error("unknown root sentinel; pos=" + to_string(r.where()));
  }

  return ret;
}

shared_ptr<JSONObject> JSONObject::parse(const char* s, size_t size) {
  StringReader r(s, size);
  return JSONObject::parse(r);
}

shared_ptr<JSONObject> JSONObject::parse(const string& s) {
  StringReader r(s.c_str(), s.size());
  return JSONObject::parse(r);
}

JSONObject::JSONObject() : value(nullptr) { }
JSONObject::JSONObject(bool x) : value(x) { }
JSONObject::JSONObject(const char* x) {
  string s(x);
  this->value = move(s);
}
JSONObject::JSONObject(const char* x, size_t size) {
  string s(x, size);
  this->value = move(s);
}
JSONObject::JSONObject(const string& x) : value(x) { }
JSONObject::JSONObject(string&& x) : value(move(x)) { }
JSONObject::JSONObject(int64_t x) : value(x) { }
JSONObject::JSONObject(double x) : value(x) { }
JSONObject::JSONObject(const list_type& x) : value(x) { }
JSONObject::JSONObject(list_type&& x) : value(move(x)) { }
JSONObject::JSONObject(const dict_type& x) : value(x) { }
JSONObject::JSONObject(dict_type&& x) : value(move(x)) { }

// non-shared_ptr constructors
JSONObject::JSONObject(const vector<JSONObject>& x) {
  list_type data;
  for (const auto& it : x) {
    data.emplace_back(new JSONObject(it));
  }
  this->value = move(data);
}

JSONObject::JSONObject(vector<JSONObject>&& x) {
  list_type data;
  for (auto& it : x) {
    data.emplace_back(new JSONObject(move(it)));
  }
  this->value = move(data);
}

JSONObject::JSONObject(const unordered_map<string, JSONObject>& x) {
  dict_type data;
  for (auto& it : x) {
    data.emplace(it.first, shared_ptr<JSONObject>(new JSONObject(it.second)));
  }
  this->value = move(data);
}

JSONObject::JSONObject(unordered_map<string, JSONObject>&& x) {
  dict_type data;
  for (auto& it : x) {
    data.emplace(
        it.first, shared_ptr<JSONObject>(new JSONObject(move(it.second))));
  }
  this->value = move(data);
}

bool JSONObject::operator==(const JSONObject& other) const {
  size_t this_index = this->value.index();
  size_t other_index = other.value.index();

  // Allow cross-type int/float comparisons
  if (this_index == 2 && other_index == 3) {
    return get<2>(this->value) == get<3>(other.value);
  } else if (this_index == 3 && other_index == 2) {
    return get<3>(this->value) == get<2>(other.value);
  }

  if (this_index != other_index) {
    return false;
  }
  switch (this_index) {
    case 0: // const void* (null)
      return true; // no data to compare
    case 1: // bool
    case 2: // int64_t
    case 3: // double
    case 4: // string
      return this->value == other.value;

    // Unlike the above types, we can't just compare the variant objects
    // directly for list_type and dict_type because the default equality
    // function compares the shared_ptrs, but we want to compare the values
    // pointed to by them.

    case 5: { // list_type
      const auto& this_list = this->as_list();
      const auto& other_list = other.as_list();
      if (this_list.size() != other_list.size()) {
        return false;
      }
      for (size_t x = 0; x < this_list.size(); x++) {
        if (*this_list[x] != *other_list[x]) {
          return false;
        }
      }
      return true;
    }

    case 6: { // dict_type
      const auto& this_dict = this->as_dict();
      const auto& other_dict = other.as_dict();
      if (this_dict.size() != other_dict.size()) {
        return false;
      }
      for (const auto& it : this_dict) {
        try {
          if (*other_dict.at(it.first) != *it.second) {
            return false;
          }
        } catch (const out_of_range& e) {
          return false;
        }
      }
      return true;
    }

    default:
      throw type_error("unknown type in operator==");
  }
}

bool JSONObject::operator!=(const JSONObject& other) const {
  return !(this->operator==(other));
}

shared_ptr<JSONObject> JSONObject::at(const string& key) {
  try {
    return this->as_dict().at(key);
  } catch (const out_of_range& e) {
    throw key_error("key not present: " + key);
  }
}

const shared_ptr<JSONObject> JSONObject::at(const string& key) const {
  try {
    return this->as_dict().at(key);
  } catch (const out_of_range& e) {
    throw key_error("key not present: " + key);
  }
}

shared_ptr<JSONObject> JSONObject::at(size_t index) {
  const auto& list = this->as_list();
  if (index >= list.size()) {
    throw index_error("array index too large");
  }
  return list[index];
}

const shared_ptr<JSONObject> JSONObject::at(size_t index) const {
  const auto& list = this->as_list();
  if (index >= list.size()) {
    throw index_error("array index too large");
  }
  return list[index];
}

unordered_map<string, shared_ptr<JSONObject>>& JSONObject::as_dict() {
  if (!this->is_dict()) {
    throw type_error("object cannot be accessed as a dict");
  }
  return get<dict_type>(this->value);
}

const unordered_map<string, shared_ptr<JSONObject>>& JSONObject::as_dict() const {
  if (!this->is_dict()) {
    throw type_error("object cannot be accessed as a dict");
  }
  return get<dict_type>(this->value);
}

vector<shared_ptr<JSONObject>>& JSONObject::as_list() {
  if (!this->is_list()) {
    throw type_error("object cannot be accessed as a list");
  }
  return get<list_type>(this->value);
}

const vector<shared_ptr<JSONObject>>& JSONObject::as_list() const {
  if (!this->is_list()) {
    throw type_error("object cannot be accessed as a list");
  }
  return get<list_type>(this->value);
}

int64_t JSONObject::as_int() const {
  if (this->is_int()) {
    return get<int64_t>(this->value);
  } else if (this->is_float()) {
    return get<double>(this->value);
  } else {
    throw type_error("object cannot be accessed as an int");
  }
}

double JSONObject::as_float() const {
  if (this->is_int()) {
    return get<int64_t>(this->value);
  } else if (this->is_float()) {
    return get<double>(this->value);
  } else {
    throw type_error("object cannot be accessed as a float");
  }
}

string& JSONObject::as_string() {
  if (!this->is_string()) {
    throw type_error("object cannot be accessed as a string");
  }
  return get<string>(this->value);
}

const string& JSONObject::as_string() const {
  if (!this->is_string()) {
    throw type_error("object cannot be accessed as a string");
  }
  return get<string>(this->value);
}

bool JSONObject::as_bool() const {
  if (!this->is_bool()) {
    throw type_error("object cannot be accessed as a bool");
  }
  return get<bool>(this->value);
}

bool JSONObject::is_dict() const {
  return holds_alternative<dict_type>(this->value);
}

bool JSONObject::is_list() const {
  return holds_alternative<list_type>(this->value);
}

bool JSONObject::is_int() const {
  return holds_alternative<int64_t>(this->value);
}

bool JSONObject::is_float() const {
  return holds_alternative<double>(this->value);
}

bool JSONObject::is_string() const {
  return holds_alternative<string>(this->value);
}

bool JSONObject::is_bool() const {
  return holds_alternative<bool>(this->value);
}

bool JSONObject::is_null() const {
  return holds_alternative<const void*>(this->value);
}

string escape_json_string(const string& s) {
  string ret;
  for (auto ch : s) {
    if (ch == '\"') {
      ret += "\\\"";
    } else if (ch == '\\') {
      ret += "\\\\";
    } else if (ch == '\b') {
      ret += "\\b";
    } else if (ch == '\f') {
      ret += "\\f";
    } else if (ch == '\n') {
      ret += "\\n";
    } else if (ch == '\r') {
      ret += "\\r";
    } else if (ch == '\t') {
      ret += "\\t";
    } else {
      ret += ch;
    }
  }
  return ret;
}

string JSONObject::serialize() const {
  size_t type_index = this->value.index();
  switch (type_index) {
    case 0: // const void* (null)
      return "null";

    case 1: // bool
      return this->as_bool() ? "true" : "false";

    case 2: // int64_t
      return to_string(this->as_int());

    case 3: { // double
      string ret = string_printf("%g", this->as_float());
      if (ret.find('.') == string::npos) {
        return ret + ".0";
      }
      return ret;
    }

    case 4: // string
      return "\"" + escape_json_string(this->as_string()) + "\"";

    case 5: { // list_type
      string ret = "[";
      for (const shared_ptr<JSONObject>& o : this->as_list()) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += o->serialize();
      }
      return ret + "]";
    }

    case 6: { // dict_type
      string ret = "{";
      for (const auto& o : this->as_dict()) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += "\"" + escape_json_string(o.first) + "\":" + o.second->serialize();
      }
      return ret + "}";
    }

    default:
      throw JSONObject::parse_error("unknown object type");
  }
}

string JSONObject::format(size_t indent) const {
  switch (this->value.index()) {
    case 0: // null
    case 1: // bool
    case 2: // int
    case 3: // float
    case 4: // string
      return this->serialize();

    case 5: { // list_type
      const auto& list = this->as_list();
      if (list.empty()) {
        return "[]";
      }

      string ret = "[";
      for (const shared_ptr<JSONObject>& o : list) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += '\n' + string(indent + 2, ' ') + o->format(indent + 2);
      }
      return ret + '\n' + string(indent, ' ') + "]";
    }

    case 6: { // dict_type
      const auto& dict = this->as_dict();
      if (dict.empty()) {
        return "{}";
      }

      string ret = "{";
      for (const auto& o : dict) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += '\n' + string(indent + 2, ' ') + "\"" + escape_json_string(o.first) + "\": " + o.second->format(indent + 2);
      }
      return ret + '\n' + string(indent, ' ') + "}";
    }
  }

  throw JSONObject::parse_error("unknown object type");
}


shared_ptr<JSONObject> make_json_null() {
  return shared_ptr<JSONObject>(new JSONObject());
}

shared_ptr<JSONObject> make_json_bool(bool x) {
  return shared_ptr<JSONObject>(new JSONObject(x));
}

shared_ptr<JSONObject> make_json_num(double x) {
  return shared_ptr<JSONObject>(new JSONObject(x));
}

shared_ptr<JSONObject> make_json_int(int64_t x) {
  return shared_ptr<JSONObject>(new JSONObject(x));
}

shared_ptr<JSONObject> make_json_str(const char* s) {
  return shared_ptr<JSONObject>(new JSONObject(s));
}

shared_ptr<JSONObject> make_json_str(const string& s) {
  return shared_ptr<JSONObject>(new JSONObject(s));
}
shared_ptr<JSONObject> make_json_str(string&& s) {
  return shared_ptr<JSONObject>(new JSONObject(move(s)));
}

shared_ptr<JSONObject> make_json_list(vector<shared_ptr<JSONObject>>&& values) {
  return shared_ptr<JSONObject>(new JSONObject(move(values)));
}

shared_ptr<JSONObject> make_json_list(
    initializer_list<shared_ptr<JSONObject>> values) {
  vector<shared_ptr<JSONObject>> vec(values);
  return shared_ptr<JSONObject>(new JSONObject(move(vec)));
}

shared_ptr<JSONObject> make_json_dict(
    initializer_list<pair<const char*, shared_ptr<JSONObject>>> values) {
  unordered_map<string, shared_ptr<JSONObject>> dict;
  for (const auto& i : values) {
    dict.emplace(i.first, i.second);
  }
  return shared_ptr<JSONObject>(new JSONObject(move(dict)));
}

shared_ptr<JSONObject> make_json_dict(
    initializer_list<pair<string, shared_ptr<JSONObject>>> values) {
  unordered_map<string, shared_ptr<JSONObject>> dict;
  for (const auto& i : values) {
    dict.emplace(i.first, i.second);
  }
  return shared_ptr<JSONObject>(new JSONObject(move(dict)));
}
