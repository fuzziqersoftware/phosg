#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Strings.hh"

using namespace std;

class Arguments {
public:
  Arguments(const char* const* argv, size_t num_args);
  Arguments(const std::vector<std::string>& args);
  Arguments(std::vector<std::string>&& args);
  Arguments(const std::string& args);

  void assert_none_unused() const;

  // Strings
  static const std::string empty_string;
  static const std::vector<std::string> empty_string_vec;

  template <typename RetT>
    requires(std::is_same_v<RetT, std::string>)
  std::vector<RetT> get_multi(const string& name) {
    std::vector<std::string> ret;
    for (auto& value : this->get_values_multi(name)) {
      ret.emplace_back(value.text);
      value.used = true;
    }
    return ret;
  }
  template <typename RetT>
    requires(std::is_same_v<RetT, std::string>)
  const RetT& get(const string& name, bool throw_if_missing = false) {
    try {
      auto& values = this->named.at(name);
      if (values.empty()) {
        // This should be impossible; there should just be no key in this->named
        // if the argument wasn't specified at all
        throw std::logic_error(Arguments::exc_prefix(name) + "argument exists but value is missing");
      }
      if (values.size() > 1) {
        throw std::out_of_range(Arguments::exc_prefix(name) + "argument has multiple values");
      }
      values[0].used = true;
      return values[0].text;
    } catch (const out_of_range&) {
      if (throw_if_missing) {
        throw std::out_of_range(Arguments::exc_prefix(name) + "argument is missing");
      } else {
        return empty_string;
      }
    }
  }
  template <typename RetT>
    requires(std::is_same_v<RetT, std::string>)
  const RetT& get(size_t position, bool throw_if_missing = true) {
    try {
      auto& arg = this->positional.at(position);
      arg.used = true;
      return arg.text;
    } catch (const out_of_range&) {
      if (throw_if_missing) {
        throw std::out_of_range(Arguments::exc_prefix(position) + "argument is missing");
      } else {
        return empty_string;
      }
    }
  }

  // Booleans (named only)
  template <typename RetT>
    requires(std::is_same_v<RetT, bool>)
  RetT get(const char* id) {
    try {
      this->get<std::string>(id, true);
      return true;
    } catch (const std::out_of_range&) {
      return false;
    }
  }

  // Integers
  enum class IntFormat {
    DEFAULT = 0,
    HEX,
    DECIMAL,
    OCTAL,
  };
  template <typename RetT>
    requires(std::is_integral_v<RetT>)
  std::vector<RetT> get_multi(const std::string& name, IntFormat format = IntFormat::DEFAULT) {
    std::vector<RetT> ret;
    for (auto& v : this->get_values_multi(name)) {
      ret.emplace_back(this->parse_int<RetT>(name, v.text, format));
      v.used = true;
    }
    return ret;
  }
  template <typename RetT, typename IdentT>
    requires(std::is_integral_v<RetT>)
  RetT get(const IdentT& id, IntFormat format = IntFormat::DEFAULT) {
    return this->parse_int<RetT>(id, this->get<std::string>(id, true), format);
  }
  template <typename RetT, typename IdentT>
    requires(std::is_integral_v<RetT>)
  RetT get(const IdentT& id, RetT default_value, IntFormat format = IntFormat::DEFAULT) {
    try {
      return this->get<RetT>(id, format);
    } catch (const std::out_of_range&) {
      return default_value;
    }
  }

  // Floats
  template <typename RetT>
    requires(std::is_floating_point_v<RetT>)
  std::vector<RetT> get_multi(const std::string& name) {
    std::vector<RetT> ret;
    for (auto& v : this->get_values_multi(name)) {
      ret.emplace_back(this->parse_float<RetT>(name, v.text));
      v.used = true;
    }
    return ret;
  }
  template <typename RetT, typename IdentT>
    requires(std::is_floating_point_v<RetT>)
  RetT get(const IdentT& id, std::optional<RetT> default_value = std::nullopt) {
    const std::string* text;
    try {
      text = &this->get<std::string>(id, true);
    } catch (const std::out_of_range&) {
      if (default_value.has_value()) {
        return *default_value;
      } else {
        throw std::out_of_range(exc_prefix(id) + "argument is missing");
      }
    }
    return this->parse_float<RetT>(id, *text);
  }

private:
  struct ArgText {
    std::string text;
    bool used;

    explicit ArgText(std::string&& text);
  };

  std::unordered_map<std::string, std::vector<ArgText>> named;
  std::vector<ArgText> positional;

  void parse(std::vector<std::string>&& args);

  inline std::vector<ArgText>& get_values_multi(const string& name) {
    try {
      return this->named.at(name);
    } catch (const out_of_range&) {
      static std::vector<ArgText> empty_vec;
      return empty_vec;
    }
  }

  static inline std::string exc_prefix(const std::string& name) {
    string ret = "(";
    ret += name;
    ret += ") ";
    return ret;
  }
  static inline std::string exc_prefix(size_t position) {
    return string_printf("(@%zu) ", position);
  }

  template <typename RetT, typename IdentT>
    requires(std::is_integral_v<RetT>)
  static RetT parse_int(const IdentT& id, const std::string& text, IntFormat format) {
    int64_t v;
    char* conversion_end;
    switch (format) {
      case IntFormat::DEFAULT:
        v = strtoull(text.c_str(), &conversion_end, 0);
        if (conversion_end == text.c_str()) {
          throw std::invalid_argument(exc_prefix(id) + "value is not an integer");
        }
        break;
      case IntFormat::HEX:
        v = strtoull(text.c_str(), &conversion_end, 16);
        if (conversion_end == text.c_str()) {
          throw std::invalid_argument(exc_prefix(id) + "value is not a hex integer");
        }
        break;
      case IntFormat::DECIMAL:
        v = strtoull(text.c_str(), &conversion_end, 10);
        if (conversion_end == text.c_str()) {
          throw std::invalid_argument(exc_prefix(id) + "value is not a decimal integer");
        }
        break;
      case IntFormat::OCTAL:
        v = strtoull(text.c_str(), &conversion_end, 8);
        if (conversion_end == text.c_str()) {
          throw std::invalid_argument(exc_prefix(id) + "value is not an octal integer");
        }
        break;
      default:
        throw std::logic_error(exc_prefix(id) + "invalid integer format");
    }
    if (*conversion_end != '\0') {
      throw std::invalid_argument(exc_prefix(id) + "extra data after integer");
    }

    uint64_t uv = static_cast<uint64_t>(v);
    if (std::is_unsigned_v<RetT>) {
      if (uv & (~mask_for_type<RetT>)) {
        throw std::invalid_argument(exc_prefix(id) + "unsigned value out of range");
      }
      return uv;
    } else {
      if (((uv & (~(mask_for_type<RetT> >> 1))) != 0) && ((uv & (~(mask_for_type<RetT> >> 1))) != (~(mask_for_type<RetT> >> 1)))) {
        throw std::invalid_argument(exc_prefix(id) + "signed value out of range");
      }
      return v;
    }
  }

  template <typename RetT, typename IdentT>
    requires(std::is_floating_point_v<RetT>)
  static RetT parse_float(const IdentT& id, const std::string& text) {
    char* conversion_end;
    double v = strtod(text.c_str(), &conversion_end);
    if (conversion_end == text.c_str()) {
      throw std::invalid_argument(exc_prefix(id) + "value is not a number");
    }
    if (*conversion_end != '\0') {
      throw std::invalid_argument(exc_prefix(id) + "extra data after number");
    }
    return v;
  }
};

std::vector<std::string> split_args(const std::string& s);
