#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "JSON.hh"
#include "UnitTest.hh"

using namespace std;

enum JSONTestEnum {
  ONE = 1,
  TWO = 2,
  THREE = 3,
};

template <>
JSONTestEnum enum_for_name<JSONTestEnum>(const char* name) {
  if (!strcmp(name, "ONE")) {
    return JSONTestEnum::ONE;
  } else if (!strcmp(name, "TWO")) {
    return JSONTestEnum::TWO;
  } else if (!strcmp(name, "THREE")) {
    return JSONTestEnum::THREE;
  } else {
    throw runtime_error("invalid JSONTestEnum value");
  }
}

template <>
const char* name_for_enum<JSONTestEnum>(JSONTestEnum v) {
  switch (v) {
    case JSONTestEnum::ONE:
      return "ONE";
    case JSONTestEnum::TWO:
      return "TWO";
    case JSONTestEnum::THREE:
      return "THREE";
    default:
      throw invalid_argument("invalid JSONTestEnum value");
  }
}

int main(int, char**) {
  uint32_t hex_option = JSON::SerializeOption::HEX_INTEGERS;
  uint32_t format_option = JSON::SerializeOption::FORMAT;
  uint32_t one_char_trivial_option = JSON::SerializeOption::ONE_CHARACTER_TRIVIAL_CONSTANTS;

  fprintf(stderr, "-- construction\n");
  JSON orig_root = JSON::dict({
      {"null", nullptr},
      {"true", true},
      {"false", false},
      {"string0", ""},
      {"string1", "v"},
      {"string2", "no special chars"},
      {"string3", "omg \"\'\\\t\n"},
      {"int0", 0},
      {"int1", 134},
      {"int2", -3214},
      {"float0", 0.0},
      {"float1", 1.4},
      {"float2", -10.5},
      {"enum", JSONTestEnum::TWO},
      {"list0", JSON::list({})},
      {"list1", JSON::list({1})},
      {"dict0", JSON::dict()},
      {"dict1", JSON::dict({{"one", 1}})},
  });
  JSON root = JSON::dict({
      {"null", nullptr},
      {"true", true},
      {"false", false},
      {"string0", ""},
      {"string1", "v"},
      {"string2", "no special chars"},
      {"string3", "omg \"\'\\\t\n"},
      {"int0", 0},
      {"int1", 134},
      {"int2", -3214},
      {"float0", 0.0},
      {"float1", 1.4},
      {"float2", -10.5},
      {"enum", JSONTestEnum::TWO},
      {"list0", JSON::list()},
      {"list1", JSON::list({1})},
      {"dict0", JSON::dict()},
      {"dict1", JSON::dict({{"one", 1}})},
  });

  fprintf(stderr, "-- equality\n");
  expect_eq(root, orig_root);
  root.emplace("extra_item", 3);
  expect_ne(root, orig_root);

  fprintf(stderr, "-- int/float equality\n");
  expect(JSON(0).is_int());
  expect(JSON(3).is_int());
  expect(JSON(0.0).is_float());
  expect(JSON(3.0).is_float());
  expect_eq(JSON(3), JSON(3.0));
  expect_eq(JSON(0.0), JSON(0));
  expect_ne(JSON(true), JSON(1));

  {
    fprintf(stderr, "-- implicit unwrap operators\n");

    bool b = root.at("true").as_bool();
    expect_eq(b, true);
    b = root.at("false").as_bool();
    expect_eq(b, false);

    string s = root.at("string2").as_string();
    expect_eq(s, "no special chars");
    s = root.at("string1").as_string();
    expect_eq(s, "v");

    int i = root.at("int1").as_int();
    expect_eq(i, 134);
    i = root.at("int0").as_int();
    expect_eq(i, 0);

    double f = root.at("float1").as_float();
    expect_eq(f, 1.4);
    f = root.at("float0").as_float();
    expect_eq(f, 0.0);
  }

  {
    fprintf(stderr, "-- get_enum\n");
    JSON enum_root = JSON::dict({{"enum1", "ONE"}, {"enum3", "THREE"}});
    JSONTestEnum e = enum_root.get_enum("enum1", JSONTestEnum::TWO);
    expect_eq(e, JSONTestEnum::ONE);
    e = enum_root.get_enum("enum3", JSONTestEnum::TWO);
    expect_eq(e, JSONTestEnum::THREE);
    e = enum_root.get_enum("missing", JSONTestEnum::TWO);
    expect_eq(e, JSONTestEnum::TWO);
  }

  fprintf(stderr, "-- get_* with default value\n");
  expect_eq(root.get_bool("true", false), true);
  expect_eq(root.get_bool("missing", false), false);
  expect_eq(root.get_string("string0", "def"), "");
  expect_eq(root.get_string("missing", "def"), "def");
  expect_eq(root.get_int("int1", 246), 134);
  expect_eq(root.get_int("missing", 246), 246);
  expect_eq(root.get_float("float1", 3.0), 1.4);
  expect_eq(root.get_float("missing", 3.0), 3.0);

  fprintf(stderr, "-- get_* without default value\n");
  expect_eq(root.get_bool("true"), true);
  expect_raises<out_of_range>([&]() {
    root.get_bool("missing");
  });
  expect_eq(root.get_string("string0"), "");
  expect_raises<out_of_range>([&]() {
    root.get_string("missing");
  });
  expect_eq(root.at("int1"), 134);
  expect_raises<out_of_range>([&]() {
    root.get_int("missing");
  });
  expect_eq(root.at("float1"), 1.4);
  expect_raises<out_of_range>([&]() {
    root.get_float("missing");
  });

  fprintf(stderr, "-- null checks\n");
  expect_eq(root.at("null").is_null(), true);
  expect_eq(root.at("true").is_null(), false);
  expect_eq(root.at("false").is_null(), false);
  expect_eq(root.at("string0").is_null(), false);
  expect_eq(root.at("string1").is_null(), false);
  expect_eq(root.at("string2").is_null(), false);
  expect_eq(root.at("string3").is_null(), false);
  expect_eq(root.at("int0").is_null(), false);
  expect_eq(root.at("int1").is_null(), false);
  expect_eq(root.at("int2").is_null(), false);
  expect_eq(root.at("float0").is_null(), false);
  expect_eq(root.at("float1").is_null(), false);
  expect_eq(root.at("float2").is_null(), false);
  expect_eq(root.at("list0").is_null(), false);
  expect_eq(root.at("list1").is_null(), false);
  expect_eq(root.at("dict0").is_null(), false);
  expect_eq(root.at("dict1").is_null(), false);

  fprintf(stderr, "-- comparison shortcut operators (null vs. x)\n");
  expect_eq(root.at("null") == nullptr, true);
  expect_eq(root.at("null") != nullptr, false);
  expect_eq(root.at("null") == true, false);
  expect_eq(root.at("null") == false, false);
  expect_eq(root.at("null") != true, true);
  expect_eq(root.at("null") != false, true);
  expect_eq(root.at("null") == "nullptr", false);
  expect_eq(root.at("null") != "nullptr", true);
  expect_eq(root.at("null") < "nullptr", false);
  expect_eq(root.at("null") <= "nullptr", false);
  expect_eq(root.at("null") > "nullptr", false);
  expect_eq(root.at("null") >= "nullptr", false);
  expect_eq(root.at("null") == string("nullptr"), false);
  expect_eq(root.at("null") != string("nullptr"), true);
  expect_eq(root.at("null") < string("nullptr"), false);
  expect_eq(root.at("null") <= string("nullptr"), false);
  expect_eq(root.at("null") > string("nullptr"), false);
  expect_eq(root.at("null") >= string("nullptr"), false);
  expect_eq(root.at("null") == 7, false);
  expect_eq(root.at("null") != 7, true);
  expect_eq(root.at("null") < 7, false);
  expect_eq(root.at("null") <= 7, false);
  expect_eq(root.at("null") > 7, false);
  expect_eq(root.at("null") >= 7, false);
  expect_eq(root.at("null") == 7.0, false);
  expect_eq(root.at("null") != 7.0, true);
  expect_eq(root.at("null") < 7.0, false);
  expect_eq(root.at("null") <= 7.0, false);
  expect_eq(root.at("null") > 7.0, false);
  expect_eq(root.at("null") >= 7.0, false);
  expect_eq(root.at("null") == JSON::list({7}), false);
  expect_eq(root.at("null") != JSON::list({7}), true);
  expect_eq(root.at("null") < JSON::list({7}), false);
  expect_eq(root.at("null") <= JSON::list({7}), false);
  expect_eq(root.at("null") > JSON::list({7}), false);
  expect_eq(root.at("null") >= JSON::list({7}), false);
  expect_eq(root.at("null") == JSON::dict({{"seven", 7}}), false);
  expect_eq(root.at("null") != JSON::dict({{"seven", 7}}), true);

  fprintf(stderr, "-- comparison shortcut operators (bool vs. x)\n");
  expect_eq(root.at("true") == nullptr, false);
  expect_eq(root.at("true") != nullptr, true);
  expect_eq(root.at("true") == true, true);
  expect_eq(root.at("true") == false, false);
  expect_eq(root.at("true") != true, false);
  expect_eq(root.at("true") != false, true);
  expect_eq(root.at("true") == "nullptr", false);
  expect_eq(root.at("true") != "nullptr", true);
  expect_eq(root.at("true") < "nullptr", false);
  expect_eq(root.at("true") <= "nullptr", false);
  expect_eq(root.at("true") > "nullptr", false);
  expect_eq(root.at("true") >= "nullptr", false);
  expect_eq(root.at("true") == string("nullptr"), false);
  expect_eq(root.at("true") != string("nullptr"), true);
  expect_eq(root.at("true") < string("nullptr"), false);
  expect_eq(root.at("true") <= string("nullptr"), false);
  expect_eq(root.at("true") > string("nullptr"), false);
  expect_eq(root.at("true") >= string("nullptr"), false);
  expect_eq(root.at("true") == 7, false);
  expect_eq(root.at("true") != 7, true);
  expect_eq(root.at("true") < 7, false);
  expect_eq(root.at("true") <= 7, false);
  expect_eq(root.at("true") > 7, false);
  expect_eq(root.at("true") >= 7, false);
  expect_eq(root.at("true") == 7.0, false);
  expect_eq(root.at("true") != 7.0, true);
  expect_eq(root.at("true") < 7.0, false);
  expect_eq(root.at("true") <= 7.0, false);
  expect_eq(root.at("true") > 7.0, false);
  expect_eq(root.at("true") >= 7.0, false);
  expect_eq(root.at("true") == JSON::list({7}), false);
  expect_eq(root.at("true") != JSON::list({7}), true);
  expect_eq(root.at("true") < JSON::list({7}), false);
  expect_eq(root.at("true") <= JSON::list({7}), false);
  expect_eq(root.at("true") > JSON::list({7}), false);
  expect_eq(root.at("true") >= JSON::list({7}), false);
  expect_eq(root.at("true") == JSON::dict({{"seven", 7}}), false);
  expect_eq(root.at("true") != JSON::dict({{"seven", 7}}), true);

  fprintf(stderr, "-- comparison shortcut operators (string vs. x)\n");
  expect_eq(root.at("string1") == nullptr, false);
  expect_eq(root.at("string1") != nullptr, true);
  expect_eq(root.at("string1") == true, false);
  expect_eq(root.at("string1") == false, false);
  expect_eq(root.at("string1") != true, true);
  expect_eq(root.at("string1") != false, true);
  expect_eq(root.at("string1") == "v", true);
  expect_eq(root.at("string1") != "v", false);
  expect_eq(root.at("string1") < "v", false);
  expect_eq(root.at("string1") < "w", true);
  expect_eq(root.at("string1") <= "v", true);
  expect_eq(root.at("string1") <= "u", false);
  expect_eq(root.at("string1") > "v", false);
  expect_eq(root.at("string1") > "u", true);
  expect_eq(root.at("string1") >= "v", true);
  expect_eq(root.at("string1") >= "w", false);
  expect_eq(root.at("string1") == string("v"), true);
  expect_eq(root.at("string1") == string("w"), false);
  expect_eq(root.at("string1") != string("v"), false);
  expect_eq(root.at("string1") != string("w"), true);
  expect_eq(root.at("string1") < string("u"), false);
  expect_eq(root.at("string1") < string("v"), false);
  expect_eq(root.at("string1") < string("w"), true);
  expect_eq(root.at("string1") <= string("u"), false);
  expect_eq(root.at("string1") <= string("v"), true);
  expect_eq(root.at("string1") <= string("w"), true);
  expect_eq(root.at("string1") > string("u"), true);
  expect_eq(root.at("string1") > string("v"), false);
  expect_eq(root.at("string1") > string("w"), false);
  expect_eq(root.at("string1") >= string("u"), true);
  expect_eq(root.at("string1") >= string("v"), true);
  expect_eq(root.at("string1") >= string("w"), false);
  expect_eq(root.at("string1") == 7, false);
  expect_eq(root.at("string1") != 7, true);
  expect_eq(root.at("string1") < 7, false);
  expect_eq(root.at("string1") <= 7, false);
  expect_eq(root.at("string1") > 7, false);
  expect_eq(root.at("string1") >= 7, false);
  expect_eq(root.at("string1") == 7.0, false);
  expect_eq(root.at("string1") != 7.0, true);
  expect_eq(root.at("string1") < 7.0, false);
  expect_eq(root.at("string1") <= 7.0, false);
  expect_eq(root.at("string1") > 7.0, false);
  expect_eq(root.at("string1") >= 7.0, false);
  expect_eq(root.at("string1") == JSON::list({7}), false);
  expect_eq(root.at("string1") != JSON::list({7}), true);
  expect_eq(root.at("string1") < JSON::list({7}), false);
  expect_eq(root.at("string1") <= JSON::list({7}), false);
  expect_eq(root.at("string1") > JSON::list({7}), false);
  expect_eq(root.at("string1") >= JSON::list({7}), false);
  expect_eq(root.at("string1") == JSON::dict({{"seven", 7}}), false);
  expect_eq(root.at("string1") != JSON::dict({{"seven", 7}}), true);

  fprintf(stderr, "-- comparison shortcut operators (int vs. x)\n");
  expect_eq(root.at("int0") == nullptr, false);
  expect_eq(root.at("int0") != nullptr, true);
  expect_eq(root.at("int0") == true, false);
  expect_eq(root.at("int0") == false, false);
  expect_eq(root.at("int0") != true, true);
  expect_eq(root.at("int0") != false, true);
  expect_eq(root.at("int0") == "v", false);
  expect_eq(root.at("int0") != "v", true);
  expect_eq(root.at("int0") < "v", false);
  expect_eq(root.at("int0") <= "v", false);
  expect_eq(root.at("int0") > "v", false);
  expect_eq(root.at("int0") >= "v", false);
  expect_eq(root.at("int0") == string("nullptr"), false);
  expect_eq(root.at("int0") != string("nullptr"), true);
  expect_eq(root.at("int0") < string("nullptr"), false);
  expect_eq(root.at("int0") <= string("nullptr"), false);
  expect_eq(root.at("int0") > string("nullptr"), false);
  expect_eq(root.at("int0") >= string("nullptr"), false);
  expect_eq(root.at("int0") == 0, true);
  expect_eq(root.at("int0") == 1, false);
  expect_eq(root.at("int0") != 0, false);
  expect_eq(root.at("int0") != 1, true);
  expect_eq(root.at("int0") < 1, true);
  expect_eq(root.at("int0") < 0, false);
  expect_eq(root.at("int0") <= 0, true);
  expect_eq(root.at("int0") <= -1, false);
  expect_eq(root.at("int0") > -1, true);
  expect_eq(root.at("int0") > 0, false);
  expect_eq(root.at("int0") >= 0, true);
  expect_eq(root.at("int0") >= 1, false);
  expect_eq(root.at("int0") == 0.0, true);
  expect_eq(root.at("int0") == 1.0, false);
  expect_eq(root.at("int0") != 0.0, false);
  expect_eq(root.at("int0") != 1.0, true);
  expect_eq(root.at("int0") < 1.0, true);
  expect_eq(root.at("int0") < 0.0, false);
  expect_eq(root.at("int0") <= 0.0, true);
  expect_eq(root.at("int0") <= -1.0, false);
  expect_eq(root.at("int0") > -1.0, true);
  expect_eq(root.at("int0") > 0.0, false);
  expect_eq(root.at("int0") >= 0.0, true);
  expect_eq(root.at("int0") >= 1.0, false);
  expect_eq(root.at("int0") == JSON::list({0}), false);
  expect_eq(root.at("int0") != JSON::list({0}), true);
  expect_eq(root.at("int0") < JSON::list({0}), false);
  expect_eq(root.at("int0") <= JSON::list({0}), false);
  expect_eq(root.at("int0") > JSON::list({0}), false);
  expect_eq(root.at("int0") >= JSON::list({0}), false);
  expect_eq(root.at("int0") == JSON::dict({{"zero", 0}}), false);
  expect_eq(root.at("int0") != JSON::dict({{"zero", 0}}), true);

  fprintf(stderr, "-- comparison shortcut operators (float vs. x)\n");
  expect_eq(root.at("float0") == nullptr, false);
  expect_eq(root.at("float0") != nullptr, true);
  expect_eq(root.at("float0") == true, false);
  expect_eq(root.at("float0") == false, false);
  expect_eq(root.at("float0") != true, true);
  expect_eq(root.at("float0") != false, true);
  expect_eq(root.at("float0") == "v", false);
  expect_eq(root.at("float0") != "v", true);
  expect_eq(root.at("float0") < "v", false);
  expect_eq(root.at("float0") <= "v", false);
  expect_eq(root.at("float0") > "v", false);
  expect_eq(root.at("float0") >= "v", false);
  expect_eq(root.at("float0") == string("nullptr"), false);
  expect_eq(root.at("float0") != string("nullptr"), true);
  expect_eq(root.at("float0") < string("nullptr"), false);
  expect_eq(root.at("float0") <= string("nullptr"), false);
  expect_eq(root.at("float0") > string("nullptr"), false);
  expect_eq(root.at("float0") >= string("nullptr"), false);
  expect_eq(root.at("float0") == 0, true);
  expect_eq(root.at("float0") == 1, false);
  expect_eq(root.at("float0") != 0, false);
  expect_eq(root.at("float0") != 1, true);
  expect_eq(root.at("float0") < 1, true);
  expect_eq(root.at("float0") < 0, false);
  expect_eq(root.at("float0") <= 0, true);
  expect_eq(root.at("float0") <= -1, false);
  expect_eq(root.at("float0") > -1, true);
  expect_eq(root.at("float0") > 0, false);
  expect_eq(root.at("float0") >= 0, true);
  expect_eq(root.at("float0") >= 1, false);
  expect_eq(root.at("float0") == 0.0, true);
  expect_eq(root.at("float0") == 1.0, false);
  expect_eq(root.at("float0") != 0.0, false);
  expect_eq(root.at("float0") != 1.0, true);
  expect_eq(root.at("float0") < 1.0, true);
  expect_eq(root.at("float0") < 0.0, false);
  expect_eq(root.at("float0") <= 0.0, true);
  expect_eq(root.at("float0") <= -1.0, false);
  expect_eq(root.at("float0") > -1.0, true);
  expect_eq(root.at("float0") > 0.0, false);
  expect_eq(root.at("float0") >= 0.0, true);
  expect_eq(root.at("float0") >= 1.0, false);
  expect_eq(root.at("float0") == JSON::list({0}), false);
  expect_eq(root.at("float0") != JSON::list({0}), true);
  expect_eq(root.at("float0") < JSON::list({0}), false);
  expect_eq(root.at("float0") <= JSON::list({0}), false);
  expect_eq(root.at("float0") > JSON::list({0}), false);
  expect_eq(root.at("float0") >= JSON::list({0}), false);
  expect_eq(root.at("float0") == JSON::dict({{"zero", 0}}), false);
  expect_eq(root.at("float0") != JSON::dict({{"zero", 0}}), true);

  fprintf(stderr, "-- comparison shortcut operators (list vs. x)\n");
  expect_eq(root.at("list1") == nullptr, false);
  expect_eq(root.at("list1") != nullptr, true);
  expect_eq(root.at("list1") == true, false);
  expect_eq(root.at("list1") == false, false);
  expect_eq(root.at("list1") != true, true);
  expect_eq(root.at("list1") != false, true);
  expect_eq(root.at("list1") == "v", false);
  expect_eq(root.at("list1") != "v", true);
  expect_eq(root.at("list1") < "v", false);
  expect_eq(root.at("list1") <= "v", false);
  expect_eq(root.at("list1") > "v", false);
  expect_eq(root.at("list1") >= "v", false);
  expect_eq(root.at("list1") == string("nullptr"), false);
  expect_eq(root.at("list1") != string("nullptr"), true);
  expect_eq(root.at("list1") < string("nullptr"), false);
  expect_eq(root.at("list1") <= string("nullptr"), false);
  expect_eq(root.at("list1") > string("nullptr"), false);
  expect_eq(root.at("list1") >= string("nullptr"), false);
  expect_eq(root.at("list1") == 0, false);
  expect_eq(root.at("list1") != 0, true);
  expect_eq(root.at("list1") < 0, false);
  expect_eq(root.at("list1") <= 0, false);
  expect_eq(root.at("list1") > 0, false);
  expect_eq(root.at("list1") >= 0, false);
  expect_eq(root.at("list1") == 0.0, false);
  expect_eq(root.at("list1") != 0.0, true);
  expect_eq(root.at("list1") < 0.0, false);
  expect_eq(root.at("list1") <= 0.0, false);
  expect_eq(root.at("list1") > 0.0, false);
  expect_eq(root.at("list1") >= 0.0, false);
  expect_eq(root.at("list1") == JSON::list({0}), false);
  expect_eq(root.at("list1") == JSON::list({1}), true);
  expect_eq(root.at("list1") == JSON::list({2}), false);
  expect_eq(root.at("list1") == JSON::list({"string"}), false);
  expect_eq(root.at("list1") != JSON::list({0}), true);
  expect_eq(root.at("list1") != JSON::list({1}), false);
  expect_eq(root.at("list1") != JSON::list({2}), true);
  expect_eq(root.at("list1") != JSON::list({"string"}), true);
  expect_eq(root.at("list1") < JSON::list({0}), false);
  expect_eq(root.at("list1") < JSON::list({1}), false);
  expect_eq(root.at("list1") < JSON::list({2}), true);
  expect_eq(root.at("list1") < JSON::list({"string"}), false);
  expect_eq(root.at("list1") <= JSON::list({0}), false);
  expect_eq(root.at("list1") <= JSON::list({1}), true);
  expect_eq(root.at("list1") <= JSON::list({2}), true);
  expect_eq(root.at("list1") <= JSON::list({"string"}), false);
  expect_eq(root.at("list1") > JSON::list({0}), true);
  expect_eq(root.at("list1") > JSON::list({1}), false);
  expect_eq(root.at("list1") > JSON::list({2}), false);
  expect_eq(root.at("list1") > JSON::list({"string"}), false);
  expect_eq(root.at("list1") >= JSON::list({0}), true);
  expect_eq(root.at("list1") >= JSON::list({1}), true);
  expect_eq(root.at("list1") >= JSON::list({2}), false);
  expect_eq(root.at("list1") >= JSON::list({"string"}), false);
  expect_eq(root.at("list1") == JSON::dict({{"zero", 0}}), false);
  expect_eq(root.at("list1") != JSON::dict({{"zero", 0}}), true);

  fprintf(stderr, "-- comparison shortcut operators (dict vs. x)\n");
  expect_eq(root.at("dict1") == nullptr, false);
  expect_eq(root.at("dict1") != nullptr, true);
  expect_eq(root.at("dict1") == true, false);
  expect_eq(root.at("dict1") == false, false);
  expect_eq(root.at("dict1") != true, true);
  expect_eq(root.at("dict1") != false, true);
  expect_eq(root.at("dict1") == "v", false);
  expect_eq(root.at("dict1") != "v", true);
  expect_eq(root.at("dict1") < "v", false);
  expect_eq(root.at("dict1") <= "v", false);
  expect_eq(root.at("dict1") > "v", false);
  expect_eq(root.at("dict1") >= "v", false);
  expect_eq(root.at("dict1") == string("nullptr"), false);
  expect_eq(root.at("dict1") != string("nullptr"), true);
  expect_eq(root.at("dict1") < string("nullptr"), false);
  expect_eq(root.at("dict1") <= string("nullptr"), false);
  expect_eq(root.at("dict1") > string("nullptr"), false);
  expect_eq(root.at("dict1") >= string("nullptr"), false);
  expect_eq(root.at("dict1") == 0, false);
  expect_eq(root.at("dict1") != 0, true);
  expect_eq(root.at("dict1") < 0, false);
  expect_eq(root.at("dict1") <= 0, false);
  expect_eq(root.at("dict1") > 0, false);
  expect_eq(root.at("dict1") >= 0, false);
  expect_eq(root.at("dict1") == 0.0, false);
  expect_eq(root.at("dict1") != 0.0, true);
  expect_eq(root.at("dict1") < 0.0, false);
  expect_eq(root.at("dict1") <= 0.0, false);
  expect_eq(root.at("dict1") > 0.0, false);
  expect_eq(root.at("dict1") >= 0.0, false);
  expect_eq(root.at("dict1") == JSON::list({1}), false);
  expect_eq(root.at("dict1") != JSON::list({1}), true);
  expect_eq(root.at("dict1") < JSON::list({1}), false);
  expect_eq(root.at("dict1") <= JSON::list({1}), false);
  expect_eq(root.at("dict1") > JSON::list({1}), false);
  expect_eq(root.at("dict1") >= JSON::list({1}), false);
  expect_eq(root.at("dict1") == JSON::dict(), false);
  expect_eq(root.at("dict1") == JSON::dict({{"zero", 0}}), false);
  expect_eq(root.at("dict1") == JSON::dict({{"one", 1}}), true);
  expect_eq(root.at("dict1") == JSON::dict({{"one", 2}}), false);
  expect_eq(root.at("dict1") != JSON::dict(), true);
  expect_eq(root.at("dict1") != JSON::dict({{"zero", 0}}), true);
  expect_eq(root.at("dict1") != JSON::dict({{"one", 1}}), false);
  expect_eq(root.at("dict1") != JSON::dict({{"one", 2}}), true);

  fprintf(stderr, "-- get()\n");
  expect_eq(root.get("true", false), true);
  expect_eq(root.get("false", true), false);
  expect_eq(root.get("missing", false), false);
  expect_eq(root.get("string1", "def"), "v");
  expect_eq(root.get("missing", "def"), "def");
  expect_eq(root.get("int1", 290), 134);
  expect_eq(root.get("missing", 290), 290);
  expect_eq(root.get("float1", 3.0), 1.4);
  expect_eq(root.get("missing", 3.0), 3.0);
  expect_eq(root.get("list1", JSON::list({2})), JSON::list({1}));
  expect_eq(root.get("missing", JSON::list({2})), JSON::list({2}));
  expect_eq(root.get("dict1", JSON::dict({{"two", 2}})), JSON::dict({{"one", 1}}));
  expect_eq(root.get("missing", JSON::dict({{"two", 2}})), JSON::dict({{"two", 2}}));

  fprintf(stderr, "-- retrieval + equality (with other JSON objects)\n");
  expect_eq(root.at("null"), JSON(nullptr));
  expect_eq(root.at("true"), JSON(true));
  expect_eq(root.at("false"), JSON(false));
  expect_eq(root.at("string0"), JSON(""));
  expect_eq(root.at("string1"), JSON("v"));
  expect_eq(root.at("string2"), JSON("no special chars"));
  expect_eq(root.at("string3"), JSON("omg \"\'\\\t\n"));
  expect_eq(root.at("int0"), JSON((int64_t)0));
  expect_eq(root.at("int1"), JSON((int64_t)134));
  expect_eq(root.at("int2"), JSON((int64_t)-3214));
  expect_eq(root.at("float0"), JSON(0.0));
  expect_eq(root.at("float1"), JSON(1.4));
  expect_eq(root.at("float2"), JSON(-10.5));
  expect_eq(root.at("list0"), JSON::list());
  expect_eq(root.at("list1"), JSON::list({1}));
  expect_eq(root.at("list1").at(0), 1);
  expect_eq(root.at("dict0"), JSON::dict());
  expect_eq(root.at("dict1"), JSON::dict({{"one", 1}}));
  expect_eq(root.at("dict1").at("one"), JSON((int64_t)1));

  fprintf(stderr, "-- retrieval + unwrap + equality\n");
  expect_eq(root.at("true").as_bool(), true);
  expect_eq(root.at("false").as_bool(), false);
  expect_eq(root.at("string0").as_string(), "");
  expect_eq(root.at("string1").as_string(), "v");
  expect_eq(root.at("string2").as_string(), "no special chars");
  expect_eq(root.at("string3").as_string(), "omg \"\'\\\t\n");
  expect_eq(root.at("int0").as_int(), 0);
  expect_eq(root.at("int1").as_int(), 134);
  expect_eq(root.at("int2").as_int(), -3214);
  expect_eq(root.at("float0").as_float(), 0.0);
  expect_eq(root.at("float1").as_float(), 1.4);
  expect_eq(root.at("float2").as_float(), -10.5);
  expect_eq(root.at("list0").as_list(), vector<unique_ptr<JSON>>());
  expect_eq(root.at("list1").as_list().size(), 1);
  expect_eq(root.at("list1").at(0).as_int(), 1);
  expect_eq(root.at("dict0").as_dict().size(), 0);
  expect_eq(root.at("dict1").at("one").as_int(), 1);
  expect_eq(root.at("dict1").as_dict().size(), 1);

  fprintf(stderr, "-- serialize\n");
  expect_eq(root.at("null").serialize(), "null");
  expect_eq(root.at("true").serialize(), "true");
  expect_eq(root.at("false").serialize(), "false");
  expect_eq(root.at("null").serialize(one_char_trivial_option), "n");
  expect_eq(root.at("true").serialize(one_char_trivial_option), "t");
  expect_eq(root.at("false").serialize(one_char_trivial_option), "f");
  expect_eq(root.at("string0").serialize(), "\"\"");
  expect_eq(root.at("string1").serialize(), "\"v\"");
  expect_eq(root.at("string2").serialize(), "\"no special chars\"");
  expect_eq(root.at("string3").serialize(), "\"omg \\\"\'\\\\\\t\\n\"");
  expect_eq(root.at("int0").serialize(), "0");
  expect_eq(root.at("int1").serialize(), "134");
  expect_eq(root.at("int2").serialize(), "-3214");
  expect_eq(root.at("int0").serialize(hex_option), "0x0");
  expect_eq(root.at("int1").serialize(hex_option), "0x86");
  expect_eq(root.at("int2").serialize(hex_option), "-0xC8E");
  expect_eq(root.at("float0").serialize(), "0.0");
  expect_eq(root.at("float1").serialize(), "1.4");
  expect_eq(root.at("float2").serialize(), "-10.5");
  expect_eq(root.at("list0").serialize(), "[]");
  expect_eq(root.at("list1").serialize(), "[1]");
  expect_eq(root.at("dict0").serialize(), "{}");
  expect_eq(root.at("dict1").serialize(), "{\"one\":1}");

  fprintf(stderr, "-- serialize (format)\n");
  expect_eq(root.at("null").serialize(format_option), "null");
  expect_eq(root.at("true").serialize(format_option), "true");
  expect_eq(root.at("false").serialize(format_option), "false");
  expect_eq(root.at("null").serialize(format_option | one_char_trivial_option), "n");
  expect_eq(root.at("true").serialize(format_option | one_char_trivial_option), "t");
  expect_eq(root.at("false").serialize(format_option | one_char_trivial_option), "f");
  expect_eq(root.at("string0").serialize(format_option), "\"\"");
  expect_eq(root.at("string1").serialize(format_option), "\"v\"");
  expect_eq(root.at("string2").serialize(format_option), "\"no special chars\"");
  expect_eq(root.at("string3").serialize(format_option), "\"omg \\\"\'\\\\\\t\\n\"");
  expect_eq(root.at("int0").serialize(format_option), "0");
  expect_eq(root.at("int1").serialize(format_option), "134");
  expect_eq(root.at("int2").serialize(format_option), "-3214");
  expect_eq(root.at("int0").serialize(format_option | hex_option), "0x0");
  expect_eq(root.at("int1").serialize(format_option | hex_option), "0x86");
  expect_eq(root.at("int2").serialize(format_option | hex_option), "-0xC8E");
  expect_eq(root.at("float0").serialize(format_option), "0.0");
  expect_eq(root.at("float1").serialize(format_option), "1.4");
  expect_eq(root.at("float2").serialize(format_option), "-10.5");
  expect_eq(root.at("list0").serialize(format_option), "[]");
  expect_eq(root.at("list1").serialize(format_option), "[\n  1\n]");
  expect_eq(root.at("dict0").serialize(format_option), "{}");
  expect_eq(root.at("dict1").serialize(format_option), "{\n  \"one\": 1\n}");

  fprintf(stderr, "-- parse\n");
  expect_eq(root.at("null"), JSON::parse("null"));
  expect_eq(root.at("true"), JSON::parse("true"));
  expect_eq(root.at("false"), JSON::parse("false"));
  expect_eq(root.at("null"), JSON::parse("n"));
  expect_eq(root.at("true"), JSON::parse("t"));
  expect_eq(root.at("false"), JSON::parse("f"));
  expect_eq(root.at("string0"), JSON::parse("\"\""));
  expect_eq(root.at("string1"), JSON::parse("\"v\""));
  expect_eq(root.at("string2"), JSON::parse("\"no special chars\""));
  expect_eq(root.at("string3"), JSON::parse("\"omg \\\"\'\\\\\\t\\n\""));
  expect_eq(root.at("int0"), JSON::parse("0"));
  expect_eq(root.at("int1"), JSON::parse("134"));
  expect_eq(root.at("int2"), JSON::parse("-3214"));
  expect_eq(root.at("int0"), JSON::parse("0x0"));
  expect_eq(root.at("int1"), JSON::parse("0x86"));
  expect_eq(root.at("int2"), JSON::parse("-0xC8E"));
  expect_eq(root.at("float0"), JSON::parse("0.0"));
  expect_eq(root.at("float1"), JSON::parse("1.4"));
  expect_eq(root.at("float2"), JSON::parse("-10.5"));
  expect_eq(root.at("list0"), JSON::parse("[]"));
  expect_eq(root.at("list1"), JSON::parse("[1]"));
  expect_eq(root.at("dict0"), JSON::parse("{}"));
  expect_eq(root.at("dict1"), JSON::parse("{\"one\":1}"));

  fprintf(stderr, "-- parse list with trailing comma\n");
  expect_eq(root.at("list1"), JSON::parse("[1,]"));
  fprintf(stderr, "-- parse dict with trailing comma\n");
  expect_eq(root.at("dict1"), JSON::parse("{\"one\":1,}"));

  fprintf(stderr, "-- serialize / parse\n");
  expect_eq(JSON::parse(root.serialize()), root);
  fprintf(stderr, "-- serialize (format) / parse\n");
  expect_eq(JSON::parse(root.serialize(JSON::SerializeOption::FORMAT)), root);

  fprintf(stderr, "-- exceptions\n");
  expect_raises<out_of_range>([&]() {
    root.at("missing_key");
  });
  expect_raises<out_of_range>([&]() {
    root.at("list1").at(2);
  });
  expect_raises<JSON::type_error>([&]() {
    root.at("list1").as_dict();
  });
  expect_raises<JSON::parse_error>([&]() {
    JSON::parse("{this isn\'t valid json}");
  });
  expect_raises<JSON::parse_error>([&]() {
    JSON::parse("{} some garbage after valid JSON");
  });
  expect_raises<JSON::parse_error>([&]() {
    JSON::parse("{} \"valid JSON after some other valid JSON\"");
  });

  expect_eq(JSON::parse("false // a comment after valid JSON").serialize(), "false");
  expect_eq(JSON::parse("false     ").serialize(), "false");

  fprintf(stderr, "-- comments\n");
  expect_eq(JSON::parse("// this is null\nnull"), nullptr);
  expect_eq(JSON::parse("[\n// empty list\n]"), JSON::list());
  expect_eq(JSON::parse("{\n// empty dict\n}"), JSON::dict());

  fprintf(stderr, "-- extensions in strict mode\n");
  expect_raises<JSON::parse_error>([&]() {
    JSON::parse("0x123", 5, true);
  });
  expect_raises<JSON::parse_error>([&]() {
    JSON::parse("// this is null\nnull", 20, true);
  });

  printf("JSONTest: all tests passed\n");
  return 0;
}
