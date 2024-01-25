#include "JSON.hh"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <map>

#include "Filesystem.hh"
#include "Strings.hh"

using namespace std;

JSON::parse_error::parse_error(const string& what) : runtime_error(what) {}
JSON::type_error::type_error(const string& what) : runtime_error(what) {}

static void skip_whitespace_and_comments(StringReader& r, bool disable_extensions) {
  bool reading_comment = false;
  while (!r.eof()) {
    char ch = r.get_s8(false);
    if (reading_comment) {
      if ((ch == '\n') || (ch == '\r')) {
        reading_comment = false;
      }
    } else {
      if (!disable_extensions && (ch == '/') && (r.pget_s8(r.where() + 1) == '/')) {
        reading_comment = true;
      } else if ((ch != ' ') && (ch != '\t') && (ch != '\r') && (ch != '\n')) {
        return;
      }
    }
    r.get_s8();
  }
}

JSON JSON::parse(StringReader& r, bool disable_extensions) {
  skip_whitespace_and_comments(r, disable_extensions);

  JSON ret;
  char root_type_ch = r.get_s8(false);
  if (root_type_ch == '{') {
    ret = JSON::dict();
    char expected_separator = '{';
    char separator = r.get_s8();
    while (separator != '}') {
      if (separator != expected_separator) {
        throw parse_error("string is not a dictionary; pos=" + to_string(r.where()));
      }
      expected_separator = ',';

      skip_whitespace_and_comments(r, disable_extensions);
      if (!disable_extensions && (r.get_s8(false) == '}')) {
        r.get_s8();
        break;
      }

      JSON key = JSON::parse(r, disable_extensions);
      skip_whitespace_and_comments(r, disable_extensions);

      if (r.get_s8() != ':') {
        throw parse_error("dictionary does not contain key/value pairs; pos=" + to_string(r.where()));
      }
      skip_whitespace_and_comments(r, disable_extensions);

      ret.emplace(std::move(key.as_string()), JSON::parse(r, disable_extensions));
      skip_whitespace_and_comments(r, disable_extensions);
      separator = r.get_s8();
    }

  } else if (root_type_ch == '[') {
    ret = JSON::list();
    char expected_separator = '[';
    char separator = r.get_s8();
    while (separator != ']') {
      if (separator != expected_separator) {
        throw parse_error("string is not a list; pos=" + to_string(r.where()));
      }
      expected_separator = ',';

      skip_whitespace_and_comments(r, disable_extensions);
      if (!disable_extensions && (r.get_s8(false) == ']')) {
        r.get_s8();
        break;
      }

      ret.emplace_back(JSON::parse(r, disable_extensions));
      skip_whitespace_and_comments(r, disable_extensions);
      separator = r.get_s8();
    }

  } else if (root_type_ch == '-' || root_type_ch == '+' || isdigit(root_type_ch)) {
    int64_t int_data;
    double float_data;
    bool is_int = true;

    bool negative = false;
    if (root_type_ch == '-') {
      negative = true;
      r.get_s8();
    }

    if (!disable_extensions &&
        ((r.where() + 2) < r.size()) &&
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
      ret = int_data;
    } else {
      ret = float_data;
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
        } else if (ch == 'x') {
          uint8_t value;
          try {
            value = value_for_hex_char(r.get_s8()) << 4;
            value |= value_for_hex_char(r.get_s8());
          } catch (const out_of_range&) {
            throw parse_error("incomplete hex escape sequence in string; pos=" + to_string(r.where()));
          }
          data.push_back(value);
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
          data.push_back(value);
        } else {
          throw parse_error("invalid escape sequence in string; pos=" + to_string(r.where()));
        }
      } else { // not an escape sequence
        data.push_back(ch);
      }
    }
    r.get_s8();

    ret = std::move(data);

  } else if (r.skip_if("null", 4) || (!disable_extensions && r.skip_if("n", 1))) {
    ret = nullptr;

  } else if (r.skip_if("true", 4) || (!disable_extensions && r.skip_if("t", 1))) {
    ret = true;

  } else if (r.skip_if("false", 5) || (!disable_extensions && r.skip_if("f", 1))) {
    ret = false;

  } else {
    throw parse_error("unknown root sentinel; pos=" + to_string(r.where()));
  }

  return ret;
}

JSON JSON::parse(const char* s, size_t size, bool disable_extensions) {
  StringReader r(s, size);
  auto ret = JSON::parse(r, disable_extensions);
  skip_whitespace_and_comments(r, disable_extensions);
  if (!r.eof()) {
    throw parse_error("unparsed data remains after value");
  }
  return ret;
}

JSON JSON::parse(const string& s, bool disable_extensions) {
  return JSON::parse(s.data(), s.size(), disable_extensions);
}

string JSON::escape_string(const string& s, StringEscapeMode mode) {
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
    } else if (static_cast<uint8_t>(ch) < 0x20) {
      if (mode != StringEscapeMode::STANDARD) {
        ret += string_printf("\\x%02hhX", ch);
      } else {
        ret += string_printf("\\u%04hhX", ch);
      }
    } else if (static_cast<uint8_t>(ch) > 0x7E) {
      if (mode == StringEscapeMode::CONTROL_ONLY) {
        ret += ch;
      } else if (mode == StringEscapeMode::HEX) {
        ret += string_printf("\\x%02hhX", ch);
      } else {
        ret += string_printf("\\u%04hhX", ch);
      }
    } else {
      ret += ch;
    }
  }
  return ret;
}

string JSON::serialize(uint32_t options, size_t indent_level) const {
  StringEscapeMode escape_mode;
  if (options & SerializeOption::ESCAPE_CONTROLS_ONLY) {
    escape_mode = StringEscapeMode::CONTROL_ONLY;
  } else if (options & SerializeOption::HEX_ESCAPE_CODES) {
    escape_mode = StringEscapeMode::HEX;
  } else {
    escape_mode = StringEscapeMode::STANDARD;
  }

  size_t type_index = this->value.index();
  switch (type_index) {
    case 0: // nullptr_t
      return (options & SerializeOption::ONE_CHARACTER_TRIVIAL_CONSTANTS) ? "n" : "null";

    case 1: // bool
      if (options & SerializeOption::ONE_CHARACTER_TRIVIAL_CONSTANTS) {
        return this->as_bool() ? "t" : "f";
      } else {
        return this->as_bool() ? "true" : "false";
      }

    case 2: { // int64_t
      int64_t v = this->as_int();
      if (options & SerializeOption::HEX_INTEGERS) {
        return v < 0 ? string_printf("-0x%" PRIX64, -v) : string_printf("0x%" PRIX64, v);
      } else {
        return to_string(this->as_int());
      }
    }

    case 3: { // double
      string ret = string_printf("%g", this->as_float());
      if (ret.find('.') == string::npos) {
        return ret + ".0";
      }
      return ret;
    }

    case 4: // string
      return "\"" + JSON::escape_string(this->as_string(), escape_mode) + "\"";

    case 5: { // list_type
      bool format = options & SerializeOption::FORMAT;

      const auto& list = this->as_list();
      if (list.empty()) {
        return "[]";
      }

      string ret = "[";
      for (const unique_ptr<JSON>& o : list) {
        if (ret.size() > 1) {
          ret += ',';
        }
        if (format) {
          ret += '\n' + string(indent_level + 2, ' ') + o->serialize(options, indent_level + 2);
        } else {
          ret += o->serialize(options);
        }
      }
      if (format) {
        return ret + '\n' + string(indent_level, ' ') + "]";
      } else {
        return ret + "]";
      }
    }

    case 6: { // dict_type
      bool format = options & SerializeOption::FORMAT;
      bool sort_keys = options & SerializeOption::SORT_DICT_KEYS;

      const auto& dict = this->as_dict();
      if (dict.empty()) {
        return "{}";
      }

      string ret = "{";
      auto add_key = [&](const string& key, const JSON& value) -> void {
        if (ret.size() > 1) {
          ret += ',';
        }
        string escaped_key = JSON::escape_string(key, escape_mode);
        if (format) {
          ret += '\n' + string(indent_level + 2, ' ') + "\"" + escaped_key + "\": " + value.serialize(options, indent_level + 2);
        } else {
          ret += "\"" + escaped_key + "\":" + value.serialize(options);
        }
      };

      if (sort_keys) {
        // Note: We're kind of breaking abstraction here (the values in sorted
        // point to objects owned by unique_ptrs) but the lifetimes of these
        // objects are assumed to span this entire function, so it's ok to do
        // this temporarily.
        map<string, JSON*> sorted;
        for (const auto& o : dict) {
          sorted.emplace(o.first, o.second.get());
        }
        for (const auto& o : sorted) {
          add_key(o.first, *o.second);
        }
      } else {
        for (const auto& o : dict) {
          add_key(o.first, *o.second);
        }
      }
      if (format) {
        return ret + '\n' + string(indent_level, ' ') + "}";
      } else {
        return ret + "}";
      }
    }

    default:
      throw parse_error("unknown object type");
  }
}

JSON::JSON() : value(nullptr) {}

JSON::JSON(nullptr_t) : value(nullptr) {}

JSON::JSON(bool x) : value(x) {}

JSON::JSON(const char* s) {
  this->value = string(s);
}

JSON::JSON(const char* s, size_t size) {
  this->value = string(s, size);
}

JSON::JSON(const string& x) : value(x) {}

JSON::JSON(string&& x) : value(std::move(x)) {}

JSON::JSON(int64_t x) : value(x) {}

JSON::JSON(double x) : value(x) {}

JSON::JSON(list_type&& x) : value(std::move(x)) {}

JSON::JSON(dict_type&& x) : value(std::move(x)) {}

JSON::JSON(const JSON& rhs) : value(nullptr) {
  this->operator=(rhs);
}

JSON& JSON::operator=(const JSON& rhs) {
  switch (rhs.value.index()) {
    case 0:
      this->value = nullptr;
      break;
    case 1:
      this->value = ::get<1>(rhs.value);
      break;
    case 2:
      this->value = ::get<2>(rhs.value);
      break;
    case 3:
      this->value = ::get<3>(rhs.value);
      break;
    case 4:
      this->value = ::get<4>(rhs.value);
      break;
    case 5: {
      this->value = vector<unique_ptr<JSON>>();
      auto& v = ::get<5>(this->value);
      v.reserve(rhs.size());
      for (const auto& item : (::get<5>(rhs.value))) {
        v.emplace_back(new JSON(*item));
      }
      break;
    }
    case 6: {
      this->value = unordered_map<string, unique_ptr<JSON>>();
      auto& v = ::get<6>(this->value);
      for (const auto& it : (::get<6>(rhs.value))) {
        v.emplace(it.first, new JSON(*it.second));
      }
      break;
    }
    default:
      throw logic_error("invalid JSON value type");
  }
  return *this;
}

partial_ordering JSON::operator<=>(const JSON& other) const {
  size_t this_index = this->value.index();
  size_t other_index = other.value.index();

  // Allow cross-type int/float comparisons
  if (this_index == 2 && other_index == 3) {
    return ::get<2>(this->value) <=> ::get<3>(other.value);
  } else if (this_index == 3 && other_index == 2) {
    return ::get<3>(this->value) <=> ::get<2>(other.value);
  }

  if (this_index != other_index) {
    return partial_ordering::unordered;
  }
  switch (this_index) {
    case 0:
      return partial_ordering::equivalent;
    case 1: {
      const bool* other_v = ::get_if<1>(&other.value);
      return (other_v == nullptr ? partial_ordering::unordered : this->operator<=>(*other_v));
    }
    case 2:
    case 3: {
      const int64_t* other_vi = ::get_if<2>(&other.value);
      if (other_vi) {
        return this->operator<=>(*other_vi);
      }
      const double* other_vf = ::get_if<3>(&other.value);
      return (other_vf == nullptr ? partial_ordering::unordered : this->operator<=>(*other_vf));
    }
    case 4: {
      const string* other_v = ::get_if<4>(&other.value);
      return (other_v == nullptr ? partial_ordering::unordered : this->operator<=>(*other_v));
    }
    case 5: {
      const list_type* other_v = ::get_if<5>(&other.value);
      return (other_v == nullptr ? partial_ordering::unordered : this->operator<=>(*other_v));
    }
    case 6: {
      const dict_type* other_v = ::get_if<6>(&other.value);
      return (other_v == nullptr ? partial_ordering::unordered : this->operator<=>(*other_v));
    }
    default:
      throw logic_error("invalid JSON value type");
  }
}

partial_ordering JSON::operator<=>(nullptr_t) const {
  const nullptr_t* stored_v = get_if<0>(&this->value);
  return (stored_v == nullptr ? partial_ordering::unordered : partial_ordering::equivalent);
}

partial_ordering JSON::operator<=>(bool v) const {
  const bool* stored_v = get_if<1>(&this->value);
  return (stored_v == nullptr ? partial_ordering::unordered : (*stored_v <=> v));
}

static inline partial_ordering partial_ordering_for_string_compare_result(int res) {
  if (res < 0) {
    return partial_ordering::less;
  } else if (res > 0) {
    return partial_ordering::greater;
  } else {
    return partial_ordering::equivalent;
  }
}

partial_ordering JSON::operator<=>(const char* v) const {
  const string* stored_v = get_if<4>(&this->value);
  return (stored_v == nullptr
          ? partial_ordering::unordered
          : partial_ordering_for_string_compare_result(stored_v->compare(v)));
}
partial_ordering JSON::operator<=>(const string& v) const {
  const string* stored_v = get_if<4>(&this->value);
  return (stored_v == nullptr
          ? partial_ordering::unordered
          : partial_ordering_for_string_compare_result(stored_v->compare(v)));
}

partial_ordering JSON::operator<=>(const list_type& v) const {
  const list_type* stored_v = get_if<5>(&this->value);
  if (stored_v == nullptr) {
    return partial_ordering::unordered;
  }
  // Note: We don't use vector::operator<=> here because the items are pointers,
  // and we want to compare the pointed-to objects instead.
  size_t max_size = min<size_t>(stored_v->size(), v.size());
  for (size_t z = 0; z < max_size; z++) {
    partial_ordering item_ret = *(*stored_v)[z] <=> *v[z];
    if (item_ret != 0) {
      return item_ret;
    }
  }
  // If we get here, then all possible items matched. If the sizes aren't equal
  // and stored_v is longer, then stored_v is greater than v; if v is longer,
  // then v is greater than stored_v.
  return stored_v->size() <=> v.size();
}

partial_ordering JSON::operator<=>(const dict_type& v) const {
  const dict_type* stored_v = get_if<6>(&this->value);
  if (stored_v == nullptr) {
    return partial_ordering::unordered;
  }
  // If the dict sizes are equal and all key/value pairs match, then the overall
  // result is equality; otherwise, it's inequality. There is no ordering for
  // dictionary-typed values.
  if (stored_v->size() != v.size()) {
    return partial_ordering::unordered;
  }
  for (const auto& it : v) {
    try {
      const auto& item = stored_v->at(it.first);
      if ((*item <=> *it.second) != partial_ordering::equivalent) {
        return partial_ordering::unordered;
      }
    } catch (const out_of_range&) {
      return partial_ordering::unordered;
    }
  }
  // If we get here, then there were the same number of keys in both maps, all
  // keys in v were also in stored_v, and the corresponding values were all
  // equal. Therefore, the overall maps are equivalent.
  return partial_ordering::equivalent;
}

JSON& JSON::at(const string& key) {
  try {
    return *this->as_dict().at(key);
  } catch (const out_of_range& e) {
    throw out_of_range("JSON key not present: " + key);
  }
}

const JSON& JSON::at(const string& key) const {
  try {
    return *this->as_dict().at(key);
  } catch (const out_of_range& e) {
    throw out_of_range("JSON key not present: " + key);
  }
}

JSON& JSON::at(size_t index) {
  auto& list = this->as_list();
  if (index >= list.size()) {
    throw out_of_range("JSON array index out of bounds");
  }
  return *list[index];
}

const JSON& JSON::at(size_t index) const {
  const auto& list = this->as_list();
  if (index >= list.size()) {
    throw out_of_range("JSON array index out of bounds");
  }
  return *list[index];
}

JSON::dict_type& JSON::as_dict() {
  if (!this->is_dict()) {
    throw type_error("JSON value cannot be accessed as a dict");
  }
  return ::get<dict_type>(this->value);
}

const JSON::dict_type& JSON::as_dict() const {
  if (!this->is_dict()) {
    throw type_error("JSON value cannot be accessed as a dict");
  }
  return ::get<dict_type>(this->value);
}

JSON::list_type& JSON::as_list() {
  if (!this->is_list()) {
    throw type_error("JSON value cannot be accessed as a list");
  }
  return ::get<list_type>(this->value);
}

const JSON::list_type& JSON::as_list() const {
  if (!this->is_list()) {
    throw type_error("JSON value cannot be accessed as a list");
  }
  return ::get<list_type>(this->value);
}

int64_t JSON::as_int() const {
  if (this->is_int()) {
    return ::get<int64_t>(this->value);
  } else if (this->is_float()) {
    return ::get<double>(this->value);
  } else {
    throw type_error("JSON value cannot be accessed as an int");
  }
}

double JSON::as_float() const {
  if (this->is_int()) {
    return ::get<int64_t>(this->value);
  } else if (this->is_float()) {
    return ::get<double>(this->value);
  } else {
    throw type_error("JSON value cannot be accessed as a float");
  }
}

string& JSON::as_string() {
  if (!this->is_string()) {
    throw type_error("JSON value cannot be accessed as a string");
  }
  return ::get<string>(this->value);
}

const string& JSON::as_string() const {
  if (!this->is_string()) {
    throw type_error("JSON value cannot be accessed as a string");
  }
  return ::get<string>(this->value);
}

bool JSON::as_bool() const {
  if (!this->is_bool()) {
    throw type_error("JSON value cannot be accessed as a bool");
  }
  return ::get<bool>(this->value);
}

size_t JSON::size() const {
  switch (this->value.index()) {
    case 5:
      return ::get<5>(this->value).size();
    case 6:
      return ::get<6>(this->value).size();
    default:
      throw type_error("cannot get size of primitive JSON value");
  }
}

bool JSON::empty() const {
  switch (this->value.index()) {
    case 5:
      return ::get<5>(this->value).empty();
    case 6:
      return ::get<6>(this->value).empty();
    default:
      throw type_error("cannot get empty property of primitive JSON value");
  }
}

void JSON::clear() {
  switch (this->value.index()) {
    case 5:
      ::get<5>(this->value).clear();
      break;
    case 6:
      ::get<6>(this->value).clear();
      break;
    default:
      throw type_error("cannot clear primitive JSON value");
  }
}
