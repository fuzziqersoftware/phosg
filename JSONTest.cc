#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include <string>

#include "JSON.hh"

using namespace std;


int main(int argc, char** argv) {

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

  // make sure == works, even recursively
  assert(root == JSONObject(members));
  members.emplace(piecewise_construct, make_tuple("extra_item"), make_tuple((int64_t)3));
  assert(root != JSONObject(members));

  // make sure retrievals work
  assert(root["null"]->is_null() == true);
  assert(root["true"]->is_null() == false);
  assert(root["false"]->is_null() == false);
  assert(root["string0"]->is_null() == false);
  assert(root["string1"]->is_null() == false);
  assert(root["string2"]->is_null() == false);
  assert(root["int0"]->is_null() == false);
  assert(root["int1"]->is_null() == false);
  assert(root["int2"]->is_null() == false);
  assert(root["float0"]->is_null() == false);
  assert(root["float1"]->is_null() == false);
  assert(root["float2"]->is_null() == false);
  assert(root["list0"]->is_null() == false);
  assert(root["list1"]->is_null() == false);
  assert(root["dict0"]->is_null() == false);
  assert(root["dict1"]->is_null() == false);

  assert(*root["null"] == JSONObject());
  assert(*root["true"] == JSONObject(true));
  assert(*root["false"] == JSONObject(false));
  assert(*root["string0"] == JSONObject(""));
  assert(*root["string1"] == JSONObject("no special chars"));
  assert(*root["string2"] == JSONObject("omg \"\'\\\t\n"));
  assert(*root["int0"] == JSONObject((int64_t)0));
  assert(*root["int1"] == JSONObject((int64_t)134));
  assert(*root["int2"] == JSONObject((int64_t)-3214));
  assert(*root["float0"] == JSONObject(0.0));
  assert(*root["float1"] == JSONObject(1.4));
  assert(*root["float2"] == JSONObject(-10.5));
  assert(*root["list0"] == JSONObject((vector<JSONObject>())));
  assert(*root["list1"] == JSONObject((vector<JSONObject>({{JSONObject((int64_t)1)}}))));
  assert(*(*root["list1"])[0] == JSONObject((int64_t)1));
  assert(*root["dict0"] == JSONObject(unordered_map<string, JSONObject>()));
  assert(*root["dict1"] == JSONObject(unordered_map<string, JSONObject>({{"1", JSONObject((int64_t)1)}})));
  assert(*(*root["dict1"])["1"] == JSONObject((int64_t)1));

  assert(root["true"]->as_bool() == true);
  assert(root["false"]->as_bool() == false);
  assert(root["string0"]->as_string() == "");
  assert(root["string1"]->as_string() == "no special chars");
  assert(root["string2"]->as_string() == "omg \"\'\\\t\n");
  assert(root["int0"]->as_int() == 0);
  assert(root["int1"]->as_int() == 134);
  assert(root["int2"]->as_int() == -3214);
  assert(root["float0"]->as_float() == 0.0);
  assert(root["float1"]->as_float() == 1.4);
  assert(root["float2"]->as_float() == -10.5);
  assert(root["list0"]->as_list() == vector<shared_ptr<JSONObject>>());
  assert(*(*root["list1"])[0] == JSONObject((int64_t)1));
  assert(root["list1"]->as_list().size() == 1);
  assert((*root["list1"])[0]->as_int() == 1);
  assert(root["dict0"]->as_dict().size() == 0);
  assert((*root["dict1"])["1"]->as_int() == 1);
  assert(root["dict1"]->as_dict().size() == 1);

  assert(root["null"]->serialize() == "null");
  assert(root["true"]->serialize() == "true");
  assert(root["false"]->serialize() == "false");
  assert(root["string0"]->serialize() == "\"\"");
  assert(root["string1"]->serialize() == "\"no special chars\"");
  assert(root["string2"]->serialize() == "\"omg \\\"\'\\\\\\t\\n\"");
  assert(root["int0"]->serialize() == "0");
  assert(root["int1"]->serialize() == "134");
  assert(root["int2"]->serialize() == "-3214");
  assert(root["float0"]->serialize() == "0.0");
  assert(root["float1"]->serialize() == "1.4");
  assert(root["float2"]->serialize() == "-10.5");
  assert(root["list0"]->serialize() == "[]");
  assert(root["list1"]->serialize() == "[1]");
  assert(root["dict0"]->serialize() == "{}");
  assert(root["dict1"]->serialize() == "{\"1\":1}");

  assert(*root["null"] == *JSONObject::parse("null"));
  assert(*root["true"] == *JSONObject::parse("true"));
  assert(*root["false"] == *JSONObject::parse("false"));
  assert(*root["string0"] == *JSONObject::parse("\"\""));
  assert(*root["string1"] == *JSONObject::parse("\"no special chars\""));
  assert(*root["string2"] == *JSONObject::parse("\"omg \\\"\'\\\\\\t\\n\""));
  assert(*root["int0"] == *JSONObject::parse("0"));
  assert(*root["int1"] == *JSONObject::parse("134"));
  assert(*root["int2"] == *JSONObject::parse("-3214"));
  assert(*root["float0"] == *JSONObject::parse("0.0"));
  assert(*root["float1"] == *JSONObject::parse("1.4"));
  assert(*root["float2"] == *JSONObject::parse("-10.5"));
  assert(*root["list0"] == *JSONObject::parse("[]"));
  assert(*root["list1"] == *JSONObject::parse("[1]"));
  assert(*root["dict0"] == *JSONObject::parse("{}"));
  assert(*root["dict1"] == *JSONObject::parse("{\"1\":1}"));

  // make sure serialize/deserialize result in the same object
  assert(*JSONObject::parse(root.serialize(), 0) == root);
  assert(*JSONObject::parse(root.format(), 0) == root);

  // make sure save/load result in the same object
  string temp_filename = string(argv[0]) + "-data.json";
  root.save(temp_filename);
  assert(*JSONObject::load(temp_filename) == root);
  unlink(temp_filename.c_str());

  root.save(temp_filename, true);
  assert(*JSONObject::load(temp_filename) == root);
  unlink(temp_filename.c_str());

  // make sure the right exceptions are thrown
  try {
    root["missing_key"];
    assert(false);
  } catch (const JSONObject::key_error& e) {
  } catch (...) {
    assert(false);
  }

  try {
    (*root["list1"])[2];
    assert(false);
  } catch (const JSONObject::index_error& e) {
  } catch (...) {
    assert(false);
  }

  try {
    root["list1"]->as_dict();
    assert(false);
  } catch (const JSONObject::type_error& e) {
  } catch (...) {
    assert(false);
  }

  try {
    JSONObject::parse("{this isn\'t valid json}", 0);
    assert(false);
  } catch (const JSONObject::parse_error& e) {
  } catch (...) {
    assert(false);
  }

  // make sure comments work
  assert(JSONObject() == *JSONObject::parse("// this is null\nnull"));
  assert(JSONObject((vector<JSONObject>())) == *JSONObject::parse(
      "[\n// empty list\n]"));
  assert(JSONObject(unordered_map<string, JSONObject>()) == *JSONObject::parse(
      "{\n// empty dict\n}"));

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
