#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Strings.hh"

using namespace std;

struct Arguments {
  Arguments(const char* const* argv, size_t num_args);
  Arguments(const std::vector<std::string>& args);
  Arguments(std::vector<std::string>&& args);
  Arguments(const std::string& args);

  void assert_none_unused() const;

  // Strings
  const std::string& get(const string& name, const std::string* default_value = nullptr);
  const std::string& get(size_t position, const std::string* default_value = nullptr);

  // Booleans (named only)
  template <typename RetT>
    requires(std::is_same_v<RetT, bool>)
  RetT get(const char* id) {
    try {
      this->get(id);
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

  template <typename RetT, typename IdentT>
    requires(std::is_integral_v<RetT>)
  RetT get(const IdentT& id, IntFormat format = IntFormat::DEFAULT) {
    return this->parse_int<RetT>(id, this->get(id), format);
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
  template <typename RetT, typename IdentT>
    requires(std::is_floating_point_v<RetT>)
  RetT get(const IdentT& id, std::optional<RetT> default_value = std::nullopt) {
    const std::string* text;
    try {
      text = &this->get(id);
    } catch (const std::out_of_range&) {
      if (default_value.has_value()) {
        return *default_value;
      } else {
        throw std::out_of_range(exc_prefix(id) + "argument is missing");
      }
    }
    return this->parse_float<RetT>(id, *text);
  }

  struct ArgText {
    std::string text;
    bool used;

    explicit ArgText(std::string&& text);
  };

  std::unordered_map<std::string, ArgText> named;
  std::vector<ArgText> positional;

private:
  void parse(std::vector<std::string>&& args);

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
          throw invalid_argument(exc_prefix(id) + "value is not an integer");
        }
        break;
      case IntFormat::HEX:
        v = strtoull(text.c_str(), &conversion_end, 16);
        if (conversion_end == text.c_str()) {
          throw invalid_argument(exc_prefix(id) + "value is not a hex integer");
        }
        break;
      case IntFormat::DECIMAL:
        v = strtoull(text.c_str(), &conversion_end, 10);
        if (conversion_end == text.c_str()) {
          throw invalid_argument(exc_prefix(id) + "value is not a decimal integer");
        }
        break;
      case IntFormat::OCTAL:
        v = strtoull(text.c_str(), &conversion_end, 8);
        if (conversion_end == text.c_str()) {
          throw invalid_argument(exc_prefix(id) + "value is not an octal integer");
        }
        break;
      default:
        throw logic_error(exc_prefix(id) + "invalid integer format");
    }
    if (*conversion_end != '\0') {
      throw invalid_argument(exc_prefix(id) + "extra data after integer");
    }

    if (std::is_unsigned_v<RetT>) {
      uint64_t uv = static_cast<uint64_t>(v);
      if (uv & (~mask_for_type<RetT>)) {
        throw invalid_argument(exc_prefix(id) + "unsigned value out of range");
      }
      return uv;
    } else {
      if ((v < sign_extend<int64_t, RetT>(msb_for_type<RetT>)) ||
          (v > sign_extend<int64_t, RetT>(~msb_for_type<RetT>))) {
        throw invalid_argument(exc_prefix(id) + "signed value out of range");
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
      throw invalid_argument(exc_prefix(id) + "value is not a number");
    }
    if (*conversion_end != '\0') {
      throw invalid_argument(exc_prefix(id) + "extra data after number");
    }
    return v;
  }
};

std::vector<std::string> split_args(const std::string& s);
