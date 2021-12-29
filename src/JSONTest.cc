#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <string>

#include "JSON.hh"

using namespace std;


int main(int, char** argv) {

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
  assert(root == JSONObject(members));
  members.emplace(piecewise_construct, make_tuple("extra_item"), make_tuple((int64_t)3));
  assert(root != JSONObject(members));

  fprintf(stderr, "-- int/float equality\n");
  assert(JSONObject((int64_t)3) == JSONObject(3.0));
  assert(JSONObject(0.0) == JSONObject((int64_t)0));
  assert(JSONObject(true) != JSONObject((int64_t)1));

  fprintf(stderr, "-- retrieval\n");
  assert(root.at("null")->is_null() == true);
  assert(root.at("true")->is_null() == false);
  assert(root.at("false")->is_null() == false);
  assert(root.at("string0")->is_null() == false);
  assert(root.at("string1")->is_null() == false);
  assert(root.at("string2")->is_null() == false);
  assert(root.at("int0")->is_null() == false);
  assert(root.at("int1")->is_null() == false);
  assert(root.at("int2")->is_null() == false);
  assert(root.at("float0")->is_null() == false);
  assert(root.at("float1")->is_null() == false);
  assert(root.at("float2")->is_null() == false);
  assert(root.at("list0")->is_null() == false);
  assert(root.at("list1")->is_null() == false);
  assert(root.at("dict0")->is_null() == false);
  assert(root.at("dict1")->is_null() == false);

  fprintf(stderr, "-- retrieval + equality\n");
  assert(*root.at("null") == JSONObject());
  assert(*root.at("true") == JSONObject(true));
  assert(*root.at("false") == JSONObject(false));
  assert(*root.at("string0") == JSONObject(""));
  assert(*root.at("string1") == JSONObject("no special chars"));
  assert(*root.at("string2") == JSONObject("omg \"\'\\\t\n"));
  assert(*root.at("int0") == JSONObject((int64_t)0));
  assert(*root.at("int1") == JSONObject((int64_t)134));
  assert(*root.at("int2") == JSONObject((int64_t)-3214));
  assert(*root.at("float0") == JSONObject(0.0));
  assert(*root.at("float1") == JSONObject(1.4));
  assert(*root.at("float2") == JSONObject(-10.5));
  assert(*root.at("list0") == JSONObject((vector<JSONObject>())));
  assert(*root.at("list1") == JSONObject((vector<JSONObject>({{JSONObject((int64_t)1)}}))));
  assert(*root.at("list1")->at(0) == JSONObject((int64_t)1));
  assert(*root.at("dict0") == JSONObject(unordered_map<string, JSONObject>()));
  assert(*root.at("dict1") == JSONObject(unordered_map<string, JSONObject>({{"1", JSONObject((int64_t)1)}})));
  assert(*root.at("dict1")->at("1") == JSONObject((int64_t)1));

  fprintf(stderr, "-- retrieval + unwrap + equality\n");
  assert(root.at("true")->as_bool() == true);
  assert(root.at("false")->as_bool() == false);
  assert(root.at("string0")->as_string() == "");
  assert(root.at("string1")->as_string() == "no special chars");
  assert(root.at("string2")->as_string() == "omg \"\'\\\t\n");
  assert(root.at("int0")->as_int() == 0);
  assert(root.at("int1")->as_int() == 134);
  assert(root.at("int2")->as_int() == -3214);
  assert(root.at("float0")->as_float() == 0.0);
  assert(root.at("float1")->as_float() == 1.4);
  assert(root.at("float2")->as_float() == -10.5);
  assert(root.at("list0")->as_list() == vector<shared_ptr<JSONObject>>());
  assert(root.at("list1")->as_list().size() == 1);
  assert(root.at("list1")->at(0)->as_int() == 1);
  assert(root.at("dict0")->as_dict().size() == 0);
  assert(root.at("dict1")->at("1")->as_int() == 1);
  assert(root.at("dict1")->as_dict().size() == 1);

  fprintf(stderr, "-- serialize\n");
  assert(root.at("null")->serialize() == "null");
  assert(root.at("true")->serialize() == "true");
  assert(root.at("false")->serialize() == "false");
  assert(root.at("string0")->serialize() == "\"\"");
  assert(root.at("string1")->serialize() == "\"no special chars\"");
  assert(root.at("string2")->serialize() == "\"omg \\\"\'\\\\\\t\\n\"");
  assert(root.at("int0")->serialize() == "0");
  assert(root.at("int1")->serialize() == "134");
  assert(root.at("int2")->serialize() == "-3214");
  assert(root.at("float0")->serialize() == "0.0");
  assert(root.at("float1")->serialize() == "1.4");
  assert(root.at("float2")->serialize() == "-10.5");
  assert(root.at("list0")->serialize() == "[]");
  assert(root.at("list1")->serialize() == "[1]");
  assert(root.at("dict0")->serialize() == "{}");
  assert(root.at("dict1")->serialize() == "{\"1\":1}");

  fprintf(stderr, "-- parse\n");
  assert(*root.at("null") == *JSONObject::parse("null"));
  assert(*root.at("true") == *JSONObject::parse("true"));
  assert(*root.at("false") == *JSONObject::parse("false"));
  assert(*root.at("string0") == *JSONObject::parse("\"\""));
  assert(*root.at("string1") == *JSONObject::parse("\"no special chars\""));
  assert(*root.at("string2") == *JSONObject::parse("\"omg \\\"\'\\\\\\t\\n\""));
  assert(*root.at("int0") == *JSONObject::parse("0"));
  assert(*root.at("int1") == *JSONObject::parse("134"));
  assert(*root.at("int2") == *JSONObject::parse("-3214"));
  assert(*root.at("float0") == *JSONObject::parse("0.0"));
  assert(*root.at("float1") == *JSONObject::parse("1.4"));
  assert(*root.at("float2") == *JSONObject::parse("-10.5"));
  assert(*root.at("list0") == *JSONObject::parse("[]"));
  assert(*root.at("list1") == *JSONObject::parse("[1]"));
  assert(*root.at("dict0") == *JSONObject::parse("{}"));
  assert(*root.at("dict1") == *JSONObject::parse("{\"1\":1}"));

  fprintf(stderr, "-- parse list with trailing comma\n");
  assert(*root.at("list1") == *JSONObject::parse("[1,]"));
  fprintf(stderr, "-- parse dict with trailing comma\n");
  assert(*root.at("dict1") == *JSONObject::parse("{\"1\":1,}"));

  fprintf(stderr, "-- serialize/parse\n");
  assert(*JSONObject::parse(root.serialize()) == root);
  fprintf(stderr, "-- format/parse\n");
  assert(*JSONObject::parse(root.format()) == root);

  fprintf(stderr, "-- exceptions\n");
  try {
    root.at("missing_key");
    assert(false);
  } catch (const JSONObject::key_error& e) {
  } catch (...) {
    assert(false);
  }

  try {
    root.at("list1")->at(2);
    assert(false);
  } catch (const JSONObject::index_error& e) {
  } catch (...) {
    assert(false);
  }

  try {
    root.at("list1")->as_dict();
    assert(false);
  } catch (const JSONObject::type_error& e) {
  } catch (...) {
    assert(false);
  }

  try {
    JSONObject::parse("{this isn\'t valid json}");
    assert(false);
  } catch (const JSONObject::parse_error& e) {
  } catch (...) {
    assert(false);
  }

  fprintf(stderr, "-- comments\n");
  assert(JSONObject() == *JSONObject::parse("// this is null\nnull"));
  assert(JSONObject((vector<JSONObject>())) == *JSONObject::parse(
      "[\n// empty list\n]"));
  assert(JSONObject(unordered_map<string, JSONObject>()) == *JSONObject::parse(
      "{\n// empty dict\n}"));

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
