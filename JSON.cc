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
JSONObject::file_error::file_error(const string& what) : runtime_error(what) { }

shared_ptr<JSONObject> JSONObject::load(const string& filename) {
  try {
    return JSONObject::parse(load_file(filename));
  } catch (const runtime_error& e) {
    throw file_error(e.what());
  }
}

void JSONObject::save(const string& filename, bool format) const {
  string data = format ? this->format() : this->serialize();
  try {
    save_file(filename, data);
  } catch (const runtime_error& e) {
    throw file_error(e.what());
  }
}

static size_t skip_whitespace_or_comment(const string& s, size_t offset) {
  bool reading_comment = false;
  while (offset < s.length()) {
    char ch = s[offset];
    if (reading_comment) {
      if ((ch == '\n') || (ch == '\r')) {
        reading_comment = false;
      }
    } else {
      if ((offset < s.length() - 1) && (ch == '/') && (s[offset + 1] == '/')) {
        reading_comment = true;
      } else if ((ch != ' ') && (ch != '\t') && (ch != '\r') && (ch != '\n')) {
        return offset;
      }
    }
    offset++;
  }
  return offset;
}

shared_ptr<JSONObject> JSONObject::parse(const string& s, size_t offset) {
  offset = skip_whitespace_or_comment(s, offset);
  if (offset >= s.length()) {
    throw parse_error("empty string");
  }
  size_t start_offset = offset;

  shared_ptr<JSONObject> ret(new JSONObject());

  if (s[offset] == '{') {
    ret->type = Dict;

    char expected_separator = '{';
    while (offset < s.length() && s[offset] != '}') {
      if (s[offset] != expected_separator) {
        throw parse_error("string is not a dictionary; pos=" + to_string(offset));
      }
      expected_separator = ',';

      offset++;
      offset = skip_whitespace_or_comment(s, offset);
      if (offset >= s.length() || s[offset] == '}') {
        break;
      }

      shared_ptr<JSONObject> key = JSONObject::parse(s, offset);
      offset += key->source_length();
      offset = skip_whitespace_or_comment(s, offset);

      if (offset >= s.length() || s[offset] != ':') {
        throw parse_error("dictionary does not contain key/value pairs; pos=" + to_string(offset));
      }
      offset++;
      offset = skip_whitespace_or_comment(s, offset);

      ret->dict_data.emplace(piecewise_construct, make_tuple(key->as_string()),
          make_tuple(JSONObject::parse(s, offset)));
      offset += ret->dict_data[key->as_string()]->source_length();
      offset = skip_whitespace_or_comment(s, offset);
    }

    if (offset >= s.length()) {
      throw parse_error("incomplete dictionary definition; pos=" + to_string(offset));
    }

    offset++;

  } else if (s[offset] == '[') {
    ret->type = List;

    char expected_separator = '[';
    while (offset < s.length() && s[offset] != ']') {
      if (s[offset] != expected_separator) {
        throw parse_error("string is not a list; pos=" + to_string(offset));
      }
      expected_separator = ',';
      offset++;
      offset = skip_whitespace_or_comment(s, offset);
      if (offset >= s.length() || s[offset] == ']') {
        break;
      }

      ret->list_data.emplace_back(JSONObject::parse(s, offset));
      offset += ret->list_data.back()->source_length();
      offset = skip_whitespace_or_comment(s, offset);
    }

    if (offset >= s.length())
      throw parse_error("incomplete list definition; pos=" + to_string(offset));

    offset++;

  } else if (s[offset] == '-' || s[offset] == '+' || isdigit(s[offset])) {
    ret->type = Integer;

    bool negative = false;
    if (s[offset] == '-') {
      negative = true;
      offset++;
    }

    if (s[offset] == '0' && s[offset + 1] == 'x') { // hexadecimal
      offset += 2;

      ret->int_data = 0;
      for (; isxdigit(s[offset]); offset++) {
        ret->int_data = (ret->int_data << 4) | value_for_hex_char(s[offset]);
      }
      ret->float_data = ret->int_data;

    } else { // decimal
      ret->int_data = 0;
      for (; isdigit(s[offset]); offset++) {
        ret->int_data = ret->int_data * 10 + (s[offset] - '0');
      }

      double this_place = 0.1;
      ret->float_data = ret->int_data;
      if (s[offset] == '.') {
        ret->type = Float;
        offset++;
        for (; isdigit(s[offset]); offset++) {
          ret->float_data += (s[offset] - '0') * this_place;
          this_place *= 0.1;
        }
      }

      if (s[offset] == 'e' || s[offset] == 'E') {
        offset++;
        bool e_negative = s[offset] == '-';
        if (s[offset] == '-' || s[offset] == '+') {
          offset++;
        }

        int e = 0;
        for (; isdigit(s[offset]); offset++) {
          e = e * 10 + (s[offset] - '0');
        }

        if (e_negative) {
          for (; e > 0; e--) {
            ret->int_data *= 0.1;
            ret->float_data *= 0.1;
          }
        } else {
          for (; e > 0; e--)
            ret->float_data *= 10;
          ret->int_data = ret->float_data;
        }
      }
    }

    if (negative) {
      ret->int_data = -ret->int_data;
      ret->float_data = -ret->float_data;
    }

  } else if (s[offset] == '\"') {
    ret->type = String;

    offset++;

    ret->string_data = "";
    while (offset < s.length() && s[offset] != '\"') {
      if (s[offset] == '\\') {
        if (offset == s.length() - 1) {
          throw parse_error("incomplete string; pos=" + to_string(offset));
        }
        offset++;
        if (s[offset] == '\"') {
          ret->string_data.push_back('\"');
        } else if (s[offset] == '\\') {
          ret->string_data.push_back('\\');
        } else if (s[offset] == '/') {
          ret->string_data.push_back('/');
        } else if (s[offset] == 'b') {
          ret->string_data.push_back('\b');
        } else if (s[offset] == 'f') {
          ret->string_data.push_back('\f');
        } else if (s[offset] == 'n') {
          ret->string_data.push_back('\n');
        } else if (s[offset] == 'r') {
          ret->string_data.push_back('\r');
        } else if (s[offset] == 't') {
          ret->string_data.push_back('\t');
        } else if (s[offset] == 'u') {
          if (offset >= s.length() - 5) {
            throw parse_error("incomplete unicode escape sequence at end of string; pos=" + to_string(offset));
          }
          uint16_t value;
          try {
            value = (value_for_hex_char(s[offset + 1]) << 12) |
                    (value_for_hex_char(s[offset + 2]) << 8) |
                    (value_for_hex_char(s[offset + 3]) << 4) |
                    value_for_hex_char(s[offset + 4]);
          } catch (const out_of_range&) {
            throw parse_error("incomplete unicode escape sequence in string; pos=" + to_string(offset));
          }
          if (value & 0xFF00) {
            throw parse_error("non-ascii unicode character sequence in string; pos=" + to_string(offset));
          }
        } else {
          throw parse_error("invalid escape sequence in string; pos=" + to_string(offset));
        }
      } else {
        ret->string_data.push_back(s[offset]);
      }
      offset++;
    }

    if (offset >= s.length())
      throw parse_error("incomplete string; pos=" + to_string(offset));

    offset++;

  } else if (!strncmp(s.c_str() + offset, "true", 4)) {
    ret->type = Bool;
    ret->bool_data = true;
    offset += 4;

  } else if (!strncmp(s.c_str() + offset, "false", 5)) {
    ret->type = Bool;
    ret->bool_data = false;
    offset += 5;

  } else if (!strncmp(s.c_str() + offset, "null", 4)) {
    ret->type = Null;
    offset += 4;

  } else {
    throw parse_error("unknown sentinel or garbage at beginning of string; pos=" + to_string(offset));
  }

  ret->consumed_characters = offset - start_offset;

  return ret;
}

JSONObject::JSONObject() : type(Null) { }
JSONObject::JSONObject(bool x) : type(Bool), bool_data(x) { }
JSONObject::JSONObject(const char* x) : type(String), string_data(x) { }
JSONObject::JSONObject(const char* x, size_t size) : type(String),
    string_data(x, size) { }
JSONObject::JSONObject(const string& x) : type(String), string_data(x) { }
JSONObject::JSONObject(string&& x) : type(String), string_data(move(x)) { }
JSONObject::JSONObject(int64_t x) : type(Integer), int_data(x),
    float_data(x) { }
JSONObject::JSONObject(double x) : type(Float), int_data(x), float_data(x) { }
JSONObject::JSONObject(const vector<shared_ptr<JSONObject>>& x) : type(List),
    list_data(x) { }
JSONObject::JSONObject(vector<shared_ptr<JSONObject>>&& x) : type(List),
    list_data(move(x)) { }
JSONObject::JSONObject(const unordered_map<string, shared_ptr<JSONObject>>& x) :
    type(Dict), dict_data(x) { }
JSONObject::JSONObject(unordered_map<string, shared_ptr<JSONObject>>&& x) :
    type(Dict), dict_data(move(x)) { }

// non-shared_ptr constructors
JSONObject::JSONObject(const vector<JSONObject>& x) : type(List) {
  for (const auto& it : x) {
    this->list_data.emplace_back(new JSONObject(it));
  }
}

JSONObject::JSONObject(vector<JSONObject>&& x) : type(List) {
  for (auto& it : x) {
    this->list_data.emplace_back(new JSONObject(move(it)));
  }
}

JSONObject::JSONObject(const unordered_map<string, JSONObject>& x) : type(Dict) {
  for (auto& it : x) {
    this->dict_data.emplace(it.first, shared_ptr<JSONObject>(new JSONObject(it.second)));
  }
}

JSONObject::JSONObject(unordered_map<string, JSONObject>&& x) : type(Dict) {
  for (auto& it : x) {
    this->dict_data.emplace(it.first, shared_ptr<JSONObject>(new JSONObject(move(it.second))));
  }
}

JSONObject::JSONObject(const JSONObject& rhs) {
  this->type = rhs.type;
  this->consumed_characters = rhs.consumed_characters;
  this->dict_data = rhs.dict_data;
  this->list_data = rhs.list_data;
  this->int_data = rhs.int_data;
  this->float_data = rhs.float_data;
  this->string_data = rhs.string_data;
  this->bool_data = rhs.bool_data;
}

JSONObject::JSONObject(JSONObject&& rhs) {
  this->type = rhs.type;
  this->consumed_characters = rhs.consumed_characters;
  this->dict_data = move(rhs.dict_data);
  this->list_data = move(rhs.list_data);
  this->int_data = rhs.int_data;
  this->float_data = rhs.float_data;
  this->string_data = move(rhs.string_data);
  this->bool_data = rhs.bool_data;
}

JSONObject& JSONObject::operator=(const JSONObject& rhs) {
  this->type = rhs.type;
  this->consumed_characters = rhs.consumed_characters;
  this->dict_data = rhs.dict_data;
  this->list_data = rhs.list_data;
  this->int_data = rhs.int_data;
  this->float_data = rhs.float_data;
  this->string_data = rhs.string_data;
  this->bool_data = rhs.bool_data;
  return *this;
}

JSONObject::~JSONObject() { }

bool JSONObject::operator==(const JSONObject& other) const {
  if (this->type != other.type) {
    return false;
  }
  switch (this->type) {
    case Null:
      return true; // no data to compare
    case Bool:
      return this->bool_data == other.bool_data;
    case Integer:
      return this->int_data == other.int_data;
    case Float:
      return this->float_data == other.float_data;
    case String:
      return this->string_data == other.string_data;

    case Dict:
      if (this->dict_data.size() != other.dict_data.size()) {
        return false;
      }
      for (const auto& it : this->dict_data) {
        try {
          if (*other.dict_data.at(it.first) != *it.second) {
            return false;
          }
        } catch (const out_of_range& e) {
          return false;
        }
      }
      return true;

    case List:
      if (this->list_data.size() != other.list_data.size()) {
        return false;
      }
      for (size_t x = 0; x < this->list_data.size(); x++) {
        if (*this->list_data[x] != *other.list_data[x]) {
          return false;
        }
      }
      return true;

    default:
      throw type_error("unknown type in operator==");
  }
}

bool JSONObject::operator!=(const JSONObject& other) const {
  return !(this->operator==(other));
}

shared_ptr<JSONObject> JSONObject::operator[](const string& key) {
  if (this->type != Dict)
    throw type_error("object cannot be accessed as a dict");
  try {
    return this->dict_data.at(key);
  } catch (const out_of_range& e) {
    throw key_error("key not present: " + key);
  }
}

const shared_ptr<JSONObject> JSONObject::operator[](const string& key) const {
  if (this->type != Dict)
    throw type_error("object cannot be accessed as a dict");
  try {
    return this->dict_data.at(key);
  } catch (const out_of_range& e) {
    throw key_error("key not present: " + key);
  }
}

shared_ptr<JSONObject> JSONObject::operator[](size_t index) {
  if (this->type != List)
    throw type_error("object cannot be accessed as a list");
  if (index >= this->list_data.size())
    throw index_error("array index too large");
  return this->list_data[index];
}

const shared_ptr<JSONObject> JSONObject::operator[](size_t index) const {
  if (this->type != List)
    throw type_error("object cannot be accessed as a list");
  if (index >= this->list_data.size())
    throw index_error("array index too large");
  return this->list_data[index];
}

shared_ptr<JSONObject> JSONObject::at(const string& key) {
  return (*this)[key];
}

const shared_ptr<JSONObject> JSONObject::at(const string& key) const {
  return (*this)[key];
}

shared_ptr<JSONObject> JSONObject::at(size_t index) {
  return (*this)[index];
}

const shared_ptr<JSONObject> JSONObject::at(size_t index) const {
  return (*this)[index];
}

unordered_map<string, shared_ptr<JSONObject>>& JSONObject::as_dict() {
  if (this->type != Dict)
    throw type_error("object cannot be accessed as a dict");
  return this->dict_data;
}

const unordered_map<string, shared_ptr<JSONObject>>& JSONObject::as_dict() const {
  if (this->type != Dict)
    throw type_error("object cannot be accessed as a dict");
  return this->dict_data;
}

vector<shared_ptr<JSONObject>>& JSONObject::as_list() {
  if (this->type != List)
    throw type_error("object cannot be accessed as a list");
  return this->list_data;
}

const vector<shared_ptr<JSONObject>>& JSONObject::as_list() const {
  if (this->type != List)
    throw type_error("object cannot be accessed as a list");
  return this->list_data;
}

int64_t JSONObject::as_int() const {
  if (this->type != Float && this->type != Integer)
    throw type_error("object cannot be accessed as an int");
  return this->int_data;
}

double JSONObject::as_float() const {
  if (this->type != Float && this->type != Integer)
    throw type_error("object cannot be accessed as a float");
  return this->float_data;
}

string& JSONObject::as_string() {
  if (this->type != String)
    throw type_error("object cannot be accessed as a string");
  return this->string_data;
}

const string& JSONObject::as_string() const {
  if (this->type != String)
    throw type_error("object cannot be accessed as a string");
  return this->string_data;
}

bool JSONObject::as_bool() const {
  if (this->type != Bool)
    throw type_error("object cannot be accessed as a bool");
  return this->bool_data;
}

bool JSONObject::is_dict() const {
  return this->type == Dict;
}

bool JSONObject::is_list() const {
  return this->type == List;
}

bool JSONObject::is_int() const {
  return this->type == Integer;
}

bool JSONObject::is_float() const {
  return this->type == Float;
}

bool JSONObject::is_string() const {
  return this->type == String;
}

bool JSONObject::is_bool() const {
  return this->type == Bool;
}

bool JSONObject::is_null() const {
  return this->type == Null;
}

int JSONObject::source_length() const {
  return this->consumed_characters;
}

static string escape_string(const string& s) {
  string ret;
  for (auto ch : s) {
    if (ch == '\"')
      ret += "\\\"";
    else if (ch == '\\')
      ret += "\\\\";
    else if (ch == '\b')
      ret += "\\b";
    else if (ch == '\f')
      ret += "\\f";
    else if (ch == '\n')
      ret += "\\n";
    else if (ch == '\r')
      ret += "\\r";
    else if (ch == '\t')
      ret += "\\t";
    else
      ret += ch;
  }
  return ret;
}

string JSONObject::serialize() const {
  switch (this->type) {
    case Null:
      return "null";

    case Bool:
      return this->bool_data ? "true" : "false";

    case String:
      return "\"" + escape_string(this->string_data) + "\"";

    case Integer:
      return to_string(this->int_data);

    case Float: {
      string ret = string_printf("%g", this->float_data);
      if (ret.find('.') == string::npos) {
        return ret + ".0";
      }
      return ret;
    }

    case List: {
      string ret = "[";
      for (const std::shared_ptr<JSONObject>& o : this->list_data) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += o->serialize();
      }
      return ret + "]";
    }

    case Dict: {
      string ret = "{";
      for (const auto& o : this->dict_data) {
        if (ret.size() > 1)
          ret += ',';
        ret += "\"" + escape_string(o.first) + "\":" + o.second->serialize();
      }
      return ret + "}";
    }
  }

  throw JSONObject::parse_error("unknown object type");
}

string JSONObject::format(size_t indent) const {
  switch (this->type) {
    case Null:
    case Bool:
    case String:
    case Integer:
    case Float:
      return this->serialize();

    case List: {
      if (this->list_data.empty()) {
        return "[]";
      }

      string ret = "[";
      for (const std::shared_ptr<JSONObject>& o : this->list_data) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += '\n' + string(indent + 2, ' ') + o->format(indent + 2);
      }
      return ret + '\n' + string(indent, ' ') + "]";
    }

    case Dict: {
      if (this->dict_data.empty()) {
        return "{}";
      }

      string ret = "{";
      for (const auto& o : this->dict_data) {
        if (ret.size() > 1) {
          ret += ',';
        }
        ret += '\n' + string(indent + 2, ' ') + "\"" + escape_string(o.first) + "\": " + o.second->format(indent + 2);
      }
      return ret + '\n' + string(indent, ' ') + "}";
    }
  }

  throw JSONObject::parse_error("unknown object type");
}
