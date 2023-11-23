#include "Arguments.hh"

using namespace std;

Arguments::ArgText::ArgText(std::string&& text)
    : text(std::move(text)),
      used(false) {}

Arguments::Arguments(const char* const* args, size_t num_args) {
  vector<string> tokens;
  for (size_t z = 0; z < num_args; z++) {
    tokens.emplace_back(args[z]);
  }
  this->parse(std::move(tokens));
}
Arguments::Arguments(const vector<string>& args) {
  auto tokens = args;
  this->parse(std::move(tokens));
}
Arguments::Arguments(vector<string>&& args) {
  this->parse(std::move(args));
}
Arguments::Arguments(const string& text) {
  auto tokens = split_args(text);
  this->parse(std::move(tokens));
}

void Arguments::assert_none_unused() const {
  for (size_t z = 0; z < this->positional.size(); z++) {
    const auto& arg = this->positional[z];
    if (!arg.used) {
      throw invalid_argument(string_printf("(@%zu) excess argument", z));
    }
  }
  for (const auto& it : this->named) {
    if (!it.second.used) {
      throw invalid_argument(string_printf("(--%s) excess argument", it.first.c_str()));
    }
  }
}

void Arguments::parse(vector<string>&& args) {
  for (string& arg : args) {
    if (!arg.empty() && (arg[0] == '-')) {
      if (arg[1] == '-') {
        size_t equal_pos = arg.find('=', 2);
        if (equal_pos != string::npos) {
          this->named.emplace(arg.substr(2, equal_pos - 2), arg.substr(equal_pos + 1));
        } else {
          this->named.emplace(arg.substr(2), "");
        }
      } else {
        for (size_t z = 1; arg[z] != '\0'; z++) {
          this->named.emplace(arg.substr(z, 1), "");
        }
      }
    } else {
      this->positional.emplace_back(std::move(arg));
    }
  }
}

const string Arguments::empty_string;
