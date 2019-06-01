#include <stdio.h>
#include <unistd.h>

#include <string>

#include "JSON.hh"
#include "JSONPickle.hh"
#include "Process.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;


#ifndef WINDOWS

string get_python_process_output(const string& script, const string* stdin_data,
    bool python3) {
  auto r = run_process({python3 ? "python3" : "python", "-c", script}, stdin_data);
  if (ends_with(r.stdout_contents, "\n")) {
    r.stdout_contents.resize(r.stdout_contents.size() - 1);
  }
  return r.stdout_contents;
}

string get_python_repr(const string& pickle_data, bool python3) {
  string script = string_printf("import pickle, sys; print(repr(pickle.load(sys.stdin%s)))",
      python3 ? ".buffer" : "");
  return get_python_process_output(script, &pickle_data, python3);
}

string get_sorted_dict_python_repr(const string& pickle_data, bool python3) {
  string script = string_printf("import pickle, sys; print(repr(sorted(pickle.load(sys.stdin%s).items())))",
      python3 ? ".buffer" : "");
  return get_python_process_output(script, &pickle_data, python3);
}

string get_python_pickled(const string& repr_data, int protocol, bool python3) {
  string script = string_printf("import pickle, sys; pickle.dump(eval(sys.stdin%s.read()), sys.stdout%s, %d)",
      python3 ? ".buffer" : "", python3 ? ".buffer" : "", protocol);
  return get_python_process_output(script, &repr_data, python3);
}


shared_ptr<JSONObject> try_parse_pickle(const string& input) {
  try {
    return parse_pickle(input);
  } catch (const exception& e) {
    fprintf(stderr, "error while parsing pickle data:\n");
    print_data(stderr, input.data(), input.size());
    throw;
  }
}


struct TestCase {
  string name;
  vector<string> pickled;
  string python_repr;
  string python3_repr;
  string sorted_python_repr; // for dicts
  JSONObject object;

  void run() const {
    fprintf(stderr, "-- %s\n", this->name.c_str());

    for (const auto& it : this->pickled) {
      fprintf(stderr, "-- %s (parse pickled representation)\n", this->name.c_str());
      auto parsed = try_parse_pickle(it);
      expect_eq(this->object, *parsed);
    }
    fprintf(stderr, "-- %s (parse serialized representation)\n", this->name.c_str());
    auto reparsed = try_parse_pickle(serialize_pickle(this->object));
    expect_eq(this->object, *reparsed);

    for (int python3 = 0; python3 < 2; python3++) {
      const string* expected_repr = NULL;
      string actual_repr;

      if (!this->sorted_python_repr.empty()) {
        fprintf(stderr, "-- %s (python%c deserialize; sorted)\n", this->name.c_str(), python3 ? '3' : '2');
        expected_repr = &this->sorted_python_repr;
        actual_repr = get_sorted_dict_python_repr(serialize_pickle(this->object), python3);

      } else if (python3 && !this->python3_repr.empty()) {
        fprintf(stderr, "-- %s (python3 deserialize; unsorted)\n", this->name.c_str());
        expected_repr = &this->python3_repr;
        actual_repr = get_python_repr(serialize_pickle(this->object), 1);

      } else {
        fprintf(stderr, "-- %s (python%c deserialize; unsorted)\n", this->name.c_str(), python3 ? '3' : '2');
        expected_repr = &this->python_repr;
        actual_repr = get_python_repr(serialize_pickle(this->object), python3);
      }

      expect_eq(*expected_repr, actual_repr);

      for (int protocol = 0; protocol <= 4; protocol++) {
        if (!python3 && (protocol > 2)) {
          continue;
        }

        fprintf(stderr, "-- %s (python%c serialize; protocol %d)\n", this->name.c_str(), python3 ? '3' : '2', protocol);
        const string* repr = (python3 && !this->python3_repr.empty()) ? &this->python3_repr : &this->python_repr;
        auto parsed = try_parse_pickle(get_python_pickled(*repr, protocol, python3));
        expect_eq(this->object, *parsed);
      }
    }
  }
};


vector<TestCase> test_cases({

  {"null",            {"N.", "\x80\x02N.", "\x80\x04N."},
                      "None", "", "",
                      JSONObject()},
  {"true",            {"I01\n.", "\x80\x02\x88.", "\x80\x04\x88."},
                      "True", "", "",
                      JSONObject(true)},
  {"false",           {"I00\n.", "\x80\x02\x89.", "\x80\x04\x89."},
                      "False", "", "",
                      JSONObject(false)},
  {"empty_string",    {"S\'\'\n.", string("U\x00.", 3), string("\x80\x02U\x00.", 5)},
                      "\'\'", "", "",
                      JSONObject("")},
  {"basic_string",    {"S\'no special chars\'\np1\n.", "U\x10no special chars.", "\x80\x02U\x10no special charsq\x01."},
                      "\'no special chars\'", "", "",
                      JSONObject("no special chars")},
  {"escaped_string",  {"S\'omg \"\\\'\\\\\\t\\n\'\np1\n.", "U\tomg \"\'\\\t\n.", "\x80\x02U\tomg \"\'\\\t\nq\x01."},
                      "\'omg \"\\\'\\\\\\t\\n\'", "", "",
                      JSONObject("omg \"\'\\\t\n")},
  {"quote_string",    {"S\"\\\'\"\np1\n.", "U\x01\'.", "\x80\x02U\x01\'."},
                      "\"\'\"", "", "",
                      JSONObject("\'")},
  {"5",               {"I5\n.", "K\x05.", "\x80\x02K\x05."},
                      "5", "", "",
                      JSONObject((int64_t)5)},
  {"-5000",           {"I-5000\n.", "Jx\xec\xff\xff.", "\x80\x02Jx\xec\xff\xff."},
                      "-5000", "", "",
                      JSONObject((int64_t)-5000)},
  {"0L",              {"L0L\n.", string("\x80\x02\x8a\x00.", 5), string("\x80\x04K\x00.", 5)},
                      "0", "", "",
                      JSONObject((int64_t)0)},
  {"-1L",             {"I-1\n.", "J\xff\xff\xff\xff.", "\x80\x02J\xff\xff\xff\xff.", "L-1L\n.", "\x80\x02\x8a\x01\xff."},
                      "-1", "", "",
                      JSONObject((int64_t)-1)},
  {"-5000L",          {"L-5000L\n.", "\x80\x02\x8a\x02x\xec."},
                      "-5000", "", "",
                      JSONObject((int64_t)-5000)},
  {"123456789012345", {"L123456789012345L\n.", "\x80\x02\x8a\x06y\xdf\r\x86Hp."},
                      "123456789012345L", "123456789012345", "",
                      JSONObject((int64_t)123456789012345)},
  {"-12345678901234", {"L-12345678901234L\n.", "\x80\x02\x8a\x06\x0e\xd0\x31\x8c\xc5\xf4."},
                      "-12345678901234L", "-12345678901234", "",
                      JSONObject((int64_t)-12345678901234)},
  {"1.2",             {"F1.2\n.", "G?\xf3\x33\x33\x33\x33\x33\x33.", "\x80\x02G?\xf3\x33\x33\x33\x33\x33\x33."},
                      "1.2", "", "",
                      JSONObject(1.2)},
  {"-4.5",            {"F-4.5\n.", string("G\xc0\x12\x00\x00\x00\x00\x00\x00.", 10), string("\x80\x02G\xc0\x12\x00\x00\x00\x00\x00\x00.", 12)},
                      "-4.5", "", "",
                      JSONObject(-4.5)},
  {"empty_list",      {"(l.", "].", "\x80\x02]."},
                      "[]", "", "",
                      JSONObject(vector<JSONObject>())},
  {"simple_list",     {"(NI01\nI13\nF2.5\nS'lolz'\np1\nt.",
                       string("(NI01\nK\rG@\x04\x00\x00\x00\x00\x00\x00U\x04lolzq\x01t.", 27),
                       string("\x80\x02(N\x88K\rG@\x04\x00\x00\x00\x00\x00\x00U\x04lolzq\x01t.", 26)},
                      "[None, True, 13, 2.5, \'lolz\']", "", "",
                      JSONObject(vector<JSONObject>({JSONObject(), JSONObject(true), JSONObject((int64_t)13), JSONObject(2.5), JSONObject("lolz")}))},
  {"empty_dict",      {"(d.", "}.", "\x80\x02}."},
                      "{}", "", "",
                      JSONObject(unordered_map<string, JSONObject>({}))},
  {"simple_dict",     {"(dp1\nS\'lolz\'\np2\nS\'omg\'\np3\nsS\'13\'\np4\nI13\nsS\'null\'\np5\nNsS\'true\'\np6\nI01\nsS\'2.5\'\np7\nF2.5\ns.",
                       string("}q\x01(U\x04lolzq\x02U\x03omgq\x03U\x02\x31\x33q\x04K\rU\x04nullq\x05NU\x04trueq\x06I01\nU\x03\x32.5q\x07G@\x04\x00\x00\x00\x00\x00\x00u.", 66),
                       string("\x80\x02}q\x01(U\x04lolzq\x02U\x03omgq\x03U\x02\x31\x33q\x04K\rU\x04nullq\x05NU\x04trueq\x06\x88U\x03\x32.5q\x07G@\x04\x00\x00\x00\x00\x00\x00u.", 65)},
                      "{'13': 13, '2.5': 2.5, 'lolz': 'omg', 'null': None, 'true': True}", "",
                      "[('13', 13), ('2.5', 2.5), ('lolz', 'omg'), ('null', None), ('true', True)]",
                      JSONObject(unordered_map<string, JSONObject>({
                        {"null", JSONObject()},
                        {"true", JSONObject(true)},
                        {"13", JSONObject((int64_t)13)},
                        {"2.5", JSONObject(2.5)},
                        {"lolz", JSONObject("omg")}
                      }))},
  {"complex_dict",    {"(dp1\nS\'1\'\n(lp2\nNaI01\nasS\'hax\'\np3\n(dp4\nS\'derp\'\np5\n(lp6\nssS\'2\'\n(lp7\n(lp8\nI13\naI14\naa(lp9\nS\'s1\'\np10\naS\'s2\'\np11\naas.",
                       "}q\x01(U\x01\x31]q\x02(NI01\neU\x03haxq\x03}q\x04U\x04\x64\x65rpq\x05]sU\x01\x32]q\x06(]q\x07(K\rK\x0e\x65]q\x08(U\x02s1q\tU\x02s2q\neeu.",
                       "\x80\x02}q\x01(U\x01\x31]q\x02(N\x88\x65U\x03haxq\x03}q\x04U\x04\x64\x65rpq\x05]sU\x01\x32]q\x06(]q\x07(K\rK\x0e\x65]q\x08(U\x02s1q\tU\x02s2q\neeu."},
                      "{'1': [None, True], '2': [[13, 14], ['s1', 's2']], 'hax': {'derp': []}}", "",
                      "[('1', [None, True]), ('2', [[13, 14], ['s1', 's2']]), ('hax', {'derp': []})]",
                      JSONObject(unordered_map<string, JSONObject>({
                        {"1", JSONObject(vector<JSONObject>({JSONObject(), JSONObject(true)}))},
                        {"2", JSONObject(vector<JSONObject>({
                          JSONObject(vector<JSONObject>({JSONObject((int64_t)13), JSONObject((int64_t)14)})),
                          JSONObject(vector<JSONObject>({JSONObject("s1"), JSONObject("s2")}))}))},
                        {"hax", JSONObject(unordered_map<string, JSONObject>({
                          {"derp", JSONObject(vector<JSONObject>())}}))}
                      }))},

  // technically \x87 isn't valid protocol 0 but I'm lazy and phosg knows what
  // to do with it anyway
  {"memo",            {"I00\np1\ng1\nI01\n\x87.", "\x80\x02\x89q\x01h\x01\x88\x87.",
                      string("\x80\x04\x89r\x01\x00\x00\x00j\x01\x00\x00\x00\x88\x87.", 16)},
                      "[False, False, True]", "", "",
                      JSONObject(vector<JSONObject>({
                          JSONObject(false), JSONObject(false), JSONObject(true)}))},

  // TODO: test advanced opcodes like 2-tuple, 3-tuple etc.
});

int main(int argc, char** argv) {
  for (const auto& c : test_cases) {
    c.run();
  }

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}

#else  // WINDOWS

int main(int argc, char** argv) {
  printf("%s: tests do not run on Windows\n", argv[0]);
  return 0;
}

#endif
