#include "Arguments.hh"

namespace phosg {

Arguments::ArgText::ArgText(std::string&& text) : text(std::move(text)), used(false) {}

Arguments::Arguments(const char* const* args, size_t num_args) {
  std::vector<std::string> tokens;
  for (size_t z = 0; z < num_args; z++) {
    tokens.emplace_back(args[z]);
  }
  this->parse(std::move(tokens));
}
Arguments::Arguments(const std::vector<std::string>& args) {
  auto tokens = args;
  this->parse(std::move(tokens));
}
Arguments::Arguments(std::vector<std::string>&& args) {
  this->parse(std::move(args));
}
Arguments::Arguments(const std::string& text) {
  auto tokens = split_args(text);
  this->parse(std::move(tokens));
}

void Arguments::assert_none_unused() const {
  for (size_t z = 0; z < this->positional.size(); z++) {
    const auto& arg = this->positional[z];
    if (!arg.used) {
      throw std::invalid_argument(std::format("(@{}) excess argument", z));
    }
  }
  for (const auto& named_it : this->named) {
    size_t index = 0;
    for (const auto& instance_it : named_it.second) {
      if (!instance_it.used) {
        throw std::invalid_argument(std::format("(--{}#{}) excess argument", named_it.first.c_str(), index));
      }
      index++;
    }
  }
}

void Arguments::parse(std::vector<std::string>&& args) {
  for (std::string& arg : args) {
    if (!arg.empty() && (arg[0] == '-')) {
      if (arg.size() == 1) {
        this->positional.emplace_back(std::move(arg));
      } else if (arg[1] == '-') {
        if (arg.size() == 2) {
          this->positional.emplace_back(std::move(arg));
        } else {
          size_t equal_pos = arg.find('=', 2);
          if (equal_pos != std::string::npos) {
            this->named[arg.substr(2, equal_pos - 2)].emplace_back(arg.substr(equal_pos + 1));
          } else {
            this->named[arg.substr(2)].emplace_back("");
          }
        }
      } else {
        for (size_t z = 1; arg[z] != '\0'; z++) {
          this->named[arg.substr(z, 1)].emplace_back("");
        }
      }
    } else {
      this->positional.emplace_back(std::move(arg));
    }
  }
}

const std::string Arguments::empty_string;

} // namespace phosg
