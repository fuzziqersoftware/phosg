#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <string>

#include "JSON.hh"
#include "UnitTest.hh"

using namespace std;

int main(int, char** argv) {
  uint32_t hex_option = JSONObject::SerializeOption::HEX_INTEGERS;
  uint32_t format_option = JSONObject::SerializeOption::FORMAT;
  uint32_t one_char_trivial_option = JSONObject::SerializeOption::ONE_CHARACTER_TRIVIAL_CONSTANTS;

  fprintf(stderr, "-- construction\n");
  unordered_map<string, JSONObject> members;
  members.emplace(piecewise_construct, make_tuple("null"), make_tuple());
  members.emplace(piecewise_construct, make_tuple("true"), make_tuple(true));
  members.emplace(piecewise_construct, make_tuple("false"), make_tuple(false));
  members.emplace(piecewise_construct, make_tuple("string0"), make_tuple(""));
  members.emplace(piecewise_construct, make_tuple("string1"), make_tuple("no special chars"));
  members.emplace(piecewise_construct, make_tuple("string2"), make_tuple("omg \"\'\\\t\n"));
  members.emplace(piecewise_construct, make_tuple("int0"), make_tuple((int64_t)0));
  members.emplace(piecewise_construct, make_tuple("int1"), make_tuple((int64_t)134));
  members.emplace(piecewise_construct, make_tuple("int2"), make_tuple((int64_t)-3214));
  members.emplace(piecewise_construct, make_tuple("float0"), make_tuple(0.0));
  members.emplace(piecewise_construct, make_tuple("float1"), make_tuple(1.4));
  members.emplace(piecewise_construct, make_tuple("float2"), make_tuple(-10.5));
  members.emplace(piecewise_construct, make_tuple("list0"), make_tuple((vector<JSONObject>())));
  members.emplace(piecewise_construct, make_tuple("list1"), make_tuple((vector<JSONObject>({{JSONObject((int64_t)1)}}))));
  members.emplace(piecewise_construct, make_tuple("dict0"), make_tuple((unordered_map<string, JSONObject>())));
  members.emplace(piecewise_construct, make_tuple("dict1"), make_tuple((unordered_map<string, JSONObject>({{"1", JSONObject((int64_t)1)}}))));
  JSONObject root(members);

  fprintf(stderr, "-- equality\n");
  expect_eq(root, JSONObject(members));
  members.emplace(piecewise_construct, make_tuple("extra_item"), make_tuple((int64_t)3));
  expect_ne(root, JSONObject(members));

  fprintf(stderr, "-- int/float equality\n");
  expect_eq(JSONObject((int64_t)3), JSONObject(3.0));
  expect_eq(JSONObject(0.0), JSONObject((int64_t)0));
  expect_ne(JSONObject(true), JSONObject((int64_t)1));

  fprintf(stderr, "-- retrieval\n");
  expect_eq(root.at("null")->is_null(), true);
  expect_eq(root.at("true")->is_null(), false);
  expect_eq(root.at("false")->is_null(), false);
  expect_eq(root.at("string0")->is_null(), false);
  expect_eq(root.at("string1")->is_null(), false);
  expect_eq(root.at("string2")->is_null(), false);
  expect_eq(root.at("int0")->is_null(), false);
  expect_eq(root.at("int1")->is_null(), false);
  expect_eq(root.at("int2")->is_null(), false);
  expect_eq(root.at("float0")->is_null(), false);
  expect_eq(root.at("float1")->is_null(), false);
  expect_eq(root.at("float2")->is_null(), false);
  expect_eq(root.at("list0")->is_null(), false);
  expect_eq(root.at("list1")->is_null(), false);
  expect_eq(root.at("dict0")->is_null(), false);
  expect_eq(root.at("dict1")->is_null(), false);

  fprintf(stderr, "-- retrieval + equality\n");
  expect_eq(*root.at("null"), JSONObject());
  expect_eq(*root.at("true"), JSONObject(true));
  expect_eq(*root.at("false"), JSONObject(false));
  expect_eq(*root.at("string0"), JSONObject(""));
  expect_eq(*root.at("string1"), JSONObject("no special chars"));
  expect_eq(*root.at("string2"), JSONObject("omg \"\'\\\t\n"));
  expect_eq(*root.at("int0"), JSONObject((int64_t)0));
  expect_eq(*root.at("int1"), JSONObject((int64_t)134));
  expect_eq(*root.at("int2"), JSONObject((int64_t)-3214));
  expect_eq(*root.at("float0"), JSONObject(0.0));
  expect_eq(*root.at("float1"), JSONObject(1.4));
  expect_eq(*root.at("float2"), JSONObject(-10.5));
  expect_eq(*root.at("list0"), JSONObject((vector<JSONObject>())));
  expect_eq(*root.at("list1"), JSONObject((vector<JSONObject>({{JSONObject((int64_t)1)}}))));
  expect_eq(*root.at("list1")->at(0), JSONObject((int64_t)1));
  expect_eq(*root.at("dict0"), JSONObject(unordered_map<string, JSONObject>()));
  expect_eq(*root.at("dict1"), JSONObject(unordered_map<string, JSONObject>({{"1", JSONObject((int64_t)1)}})));
  expect_eq(*root.at("dict1")->at("1"), JSONObject((int64_t)1));

  fprintf(stderr, "-- retrieval + unwrap + equality\n");
  expect_eq(root.at("true")->as_bool(), true);
  expect_eq(root.at("false")->as_bool(), false);
  expect_eq(root.at("string0")->as_string(), "");
  expect_eq(root.at("string1")->as_string(), "no special chars");
  expect_eq(root.at("string2")->as_string(), "omg \"\'\\\t\n");
  expect_eq(root.at("int0")->as_int(), 0);
  expect_eq(root.at("int1")->as_int(), 134);
  expect_eq(root.at("int2")->as_int(), -3214);
  expect_eq(root.at("float0")->as_float(), 0.0);
  expect_eq(root.at("float1")->as_float(), 1.4);
  expect_eq(root.at("float2")->as_float(), -10.5);
  expect_eq(root.at("list0")->as_list(), vector<shared_ptr<JSONObject>>());
  expect_eq(root.at("list1")->as_list().size(), 1);
  expect_eq(root.at("list1")->at(0)->as_int(), 1);
  expect_eq(root.at("dict0")->as_dict().size(), 0);
  expect_eq(root.at("dict1")->at("1")->as_int(), 1);
  expect_eq(root.at("dict1")->as_dict().size(), 1);

  fprintf(stderr, "-- serialize\n");
  expect_eq(root.at("null")->serialize(), "null");
  expect_eq(root.at("true")->serialize(), "true");
  expect_eq(root.at("false")->serialize(), "false");
  expect_eq(root.at("null")->serialize(one_char_trivial_option), "n");
  expect_eq(root.at("true")->serialize(one_char_trivial_option), "t");
  expect_eq(root.at("false")->serialize(one_char_trivial_option), "f");
  expect_eq(root.at("string0")->serialize(), "\"\"");
  expect_eq(root.at("string1")->serialize(), "\"no special chars\"");
  expect_eq(root.at("string2")->serialize(), "\"omg \\\"\'\\\\\\t\\n\"");
  expect_eq(root.at("int0")->serialize(), "0");
  expect_eq(root.at("int1")->serialize(), "134");
  expect_eq(root.at("int2")->serialize(), "-3214");
  expect_eq(root.at("int0")->serialize(hex_option), "0x0");
  expect_eq(root.at("int1")->serialize(hex_option), "0x86");
  expect_eq(root.at("int2")->serialize(hex_option), "-0xC8E");
  expect_eq(root.at("float0")->serialize(), "0.0");
  expect_eq(root.at("float1")->serialize(), "1.4");
  expect_eq(root.at("float2")->serialize(), "-10.5");
  expect_eq(root.at("list0")->serialize(), "[]");
  expect_eq(root.at("list1")->serialize(), "[1]");
  expect_eq(root.at("dict0")->serialize(), "{}");
  expect_eq(root.at("dict1")->serialize(), "{\"1\":1}");

  fprintf(stderr, "-- serialize(format)\n");
  expect_eq(root.at("null")->serialize(format_option), "null");
  expect_eq(root.at("true")->serialize(format_option), "true");
  expect_eq(root.at("false")->serialize(format_option), "false");
  expect_eq(root.at("null")->serialize(format_option | one_char_trivial_option), "n");
  expect_eq(root.at("true")->serialize(format_option | one_char_trivial_option), "t");
  expect_eq(root.at("false")->serialize(format_option | one_char_trivial_option), "f");
  expect_eq(root.at("string0")->serialize(format_option), "\"\"");
  expect_eq(root.at("string1")->serialize(format_option), "\"no special chars\"");
  expect_eq(root.at("string2")->serialize(format_option), "\"omg \\\"\'\\\\\\t\\n\"");
  expect_eq(root.at("int0")->serialize(format_option), "0");
  expect_eq(root.at("int1")->serialize(format_option), "134");
  expect_eq(root.at("int2")->serialize(format_option), "-3214");
  expect_eq(root.at("int0")->serialize(format_option | hex_option), "0x0");
  expect_eq(root.at("int1")->serialize(format_option | hex_option), "0x86");
  expect_eq(root.at("int2")->serialize(format_option | hex_option), "-0xC8E");
  expect_eq(root.at("float0")->serialize(format_option), "0.0");
  expect_eq(root.at("float1")->serialize(format_option), "1.4");
  expect_eq(root.at("float2")->serialize(format_option), "-10.5");
  expect_eq(root.at("list0")->serialize(format_option), "[]");
  expect_eq(root.at("list1")->serialize(format_option), "[\n  1\n]");
  expect_eq(root.at("dict0")->serialize(format_option), "{}");
  expect_eq(root.at("dict1")->serialize(format_option), "{\n  \"1\": 1\n}");

  fprintf(stderr, "-- parse\n");
  expect_eq(*root.at("null"), *JSONObject::parse("null"));
  expect_eq(*root.at("true"), *JSONObject::parse("true"));
  expect_eq(*root.at("false"), *JSONObject::parse("false"));
  expect_eq(*root.at("null"), *JSONObject::parse("n"));
  expect_eq(*root.at("true"), *JSONObject::parse("t"));
  expect_eq(*root.at("false"), *JSONObject::parse("f"));
  expect_eq(*root.at("string0"), *JSONObject::parse("\"\""));
  expect_eq(*root.at("string1"), *JSONObject::parse("\"no special chars\""));
  expect_eq(*root.at("string2"), *JSONObject::parse("\"omg \\\"\'\\\\\\t\\n\""));
  expect_eq(*root.at("int0"), *JSONObject::parse("0"));
  expect_eq(*root.at("int1"), *JSONObject::parse("134"));
  expect_eq(*root.at("int2"), *JSONObject::parse("-3214"));
  expect_eq(*root.at("int0"), *JSONObject::parse("0x0"));
  expect_eq(*root.at("int1"), *JSONObject::parse("0x86"));
  expect_eq(*root.at("int2"), *JSONObject::parse("-0xC8E"));
  expect_eq(*root.at("float0"), *JSONObject::parse("0.0"));
  expect_eq(*root.at("float1"), *JSONObject::parse("1.4"));
  expect_eq(*root.at("float2"), *JSONObject::parse("-10.5"));
  expect_eq(*root.at("list0"), *JSONObject::parse("[]"));
  expect_eq(*root.at("list1"), *JSONObject::parse("[1]"));
  expect_eq(*root.at("dict0"), *JSONObject::parse("{}"));
  expect_eq(*root.at("dict1"), *JSONObject::parse("{\"1\":1}"));

  fprintf(stderr, "-- parse list with trailing comma\n");
  expect_eq(*root.at("list1"), *JSONObject::parse("[1,]"));
  fprintf(stderr, "-- parse dict with trailing comma\n");
  expect_eq(*root.at("dict1"), *JSONObject::parse("{\"1\":1,}"));

  fprintf(stderr, "-- serialize/parse\n");
  expect_eq(*JSONObject::parse(root.serialize()), root);
  fprintf(stderr, "-- serialize(format)/parse\n");
  expect_eq(*JSONObject::parse(root.serialize(JSONObject::SerializeOption::FORMAT)), root);

  fprintf(stderr, "-- exceptions\n");
  expect_raises<JSONObject::key_error>([&]() {
    root.at("missing_key");
  });
  expect_raises<JSONObject::index_error>([&]() {
    root.at("list1")->at(2);
  });
  expect_raises<JSONObject::type_error>([&]() {
    root.at("list1")->as_dict();
  });
  expect_raises<JSONObject::parse_error>([&]() {
    JSONObject::parse("{this isn\'t valid json}");
  });
  expect_raises<JSONObject::parse_error>([&]() {
    JSONObject::parse("{} some garbage after valid JSON");
  });
  expect_raises<JSONObject::parse_error>([&]() {
    JSONObject::parse("{} \"valid JSON after some other valid JSON\"");
  });

  expect_eq(JSONObject::parse("false // a comment after valid JSON")->serialize(), "false");
  expect_eq(JSONObject::parse("false     ")->serialize(), "false");

  fprintf(stderr, "-- comments\n");
  expect_eq(JSONObject(), *JSONObject::parse("// this is null\nnull"));
  expect_eq(JSONObject((vector<JSONObject>())), *JSONObject::parse("[\n// empty list\n]"));
  expect_eq(JSONObject(unordered_map<string, JSONObject>()), *JSONObject::parse("{\n// empty dict\n}"));

  fprintf(stderr, "-- extensions in strict mode\n");
  expect_raises<JSONObject::parse_error>([&]() {
    JSONObject::parse("0x123", 5, true);
  });
  expect_raises<JSONObject::parse_error>([&]() {
    JSONObject::parse("// this is null\nnull", 20, true);
  });

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
