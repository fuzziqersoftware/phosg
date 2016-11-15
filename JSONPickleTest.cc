#include <stdio.h>
#include <unistd.h>

#include <string>

#include "JSON.hh"
#include "JSONPickle.hh"
#include "Process.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;


string get_python_process_output(const string& script, const string* stdin_data) {
  auto r = run_process({"python", "-c", script}, stdin_data);
  if (ends_with(r.stdout_contents, "\n")) {
    r.stdout_contents.resize(r.stdout_contents.size() - 1);
  }
  return r.stdout_contents;
}

string get_python_repr(const string& pickle_data) {
  return get_python_process_output("import pickle, sys; print(repr(pickle.load(sys.stdin)))",
      &pickle_data);
}

string get_sorted_dict_python_repr(const string& pickle_data) {
  return get_python_process_output("import pickle, sys; print(repr(sorted(pickle.load(sys.stdin).items())))",
      &pickle_data);
}


int main(int argc, char** argv) {

  // null
  {
    JSONObject o;
    expect_eq(parse_pickle("N."), o);
    expect_eq(parse_pickle("\x80\x02N."), o);
    expect_eq(serialize_pickle(o), "\x80\x02N.");
    expect_eq(parse_pickle(serialize_pickle(o)), o);
    expect_eq(get_python_repr(serialize_pickle(o)), "None");
  }

  // bools
  {
    JSONObject t(true);
    JSONObject f(false);
    expect_eq(parse_pickle("I01\n."), t);
    expect_eq(parse_pickle("\x80\x02\x88."), t);
    expect_eq(parse_pickle("I00\n."), f);
    expect_eq(parse_pickle("\x80\x02\x89."), f);
    expect_eq(serialize_pickle(t), "\x80\x02\x88.");
    expect_eq(serialize_pickle(f), "\x80\x02\x89.");
    expect_eq(parse_pickle(serialize_pickle(t)), t);
    expect_eq(parse_pickle(serialize_pickle(f)), f);
    expect_eq(get_python_repr(serialize_pickle(t)), "True");
    expect_eq(get_python_repr(serialize_pickle(f)), "False");
  }

  // strings
  {
    JSONObject e("");
    JSONObject n("no special chars");
    JSONObject s("omg \"\'\\\t\n");
    expect_eq(parse_pickle("S\'\'\n."), e);
    expect_eq(parse_pickle("U\x00.", 3), e);
    expect_eq(parse_pickle("\x80\x02U\x00.", 5), e);
    expect_eq(parse_pickle("S\'no special chars\'\np1\n."), n);
    expect_eq(parse_pickle("U\x10no special charsq\x01."), n);
    expect_eq(parse_pickle("\x80\x02U\x10no special charsq\x01."), n);
    expect_eq(parse_pickle("S\'omg \"\\\'\\\\\\t\\n\'\np1\n."), s);
    expect_eq(parse_pickle("U\tomg \"\'\\\t\nq\x01."), s);
    expect_eq(parse_pickle("\x80\x02U\tomg \"\'\\\t\nq\x01."), s);
    expect_eq(serialize_pickle(e), string("\x80\x02U\x00.", 5));
    expect_eq(serialize_pickle(n), "\x80\x02U\x10no special chars.");
    expect_eq(serialize_pickle(s), "\x80\x02U\tomg \"\'\\\t\n.");
    expect_eq(parse_pickle(serialize_pickle(e)), e);
    expect_eq(parse_pickle(serialize_pickle(n)), n);
    expect_eq(parse_pickle(serialize_pickle(s)), s);
    expect_eq(get_python_repr(serialize_pickle(e)), "\'\'");
    expect_eq(get_python_repr(serialize_pickle(n)), "\'no special chars\'");
    expect_eq(get_python_repr(serialize_pickle(s)), "\'omg \"\\\'\\\\\\t\\n\'");
  }

  // integers
  {
    JSONObject f((int64_t)5);
    JSONObject nft((int64_t)-5000);
    expect_eq(parse_pickle("I5\n."), f);
    expect_eq(parse_pickle("K\x05."), f);
    expect_eq(parse_pickle("\x80\x02K\x05."), f);
    expect_eq(parse_pickle("I-5000\n."), nft);
    expect_eq(parse_pickle("Jx\xec\xff\xff."), nft);
    expect_eq(parse_pickle("\x80\x02Jx\xec\xff\xff."), nft);
    expect_eq(serialize_pickle(f), "\x80\x02K\x05.");
    expect_eq(serialize_pickle(nft), "\x80\x02Jx\xec\xff\xff.");
    expect_eq(parse_pickle(serialize_pickle(f)), f);
    expect_eq(parse_pickle(serialize_pickle(nft)), nft);
    expect_eq(get_python_repr(serialize_pickle(f)), "5");
    expect_eq(get_python_repr(serialize_pickle(nft)), "-5000");
  }

  // floats
  {
    JSONObject ot(1.2);
    JSONObject nff(-4.5);
    expect_eq(parse_pickle("F1.2\n."), ot);
    expect_eq(parse_pickle("G?\xf3\x33\x33\x33\x33\x33\x33."), ot);
    expect_eq(parse_pickle("\x80\x02G?\xf3\x33\x33\x33\x33\x33\x33."), ot);
    expect_eq(parse_pickle("F-4.5\n."), nff);
    expect_eq(parse_pickle("G\xc0\x12\x00\x00\x00\x00\x00\x00.", 10), nff);
    expect_eq(parse_pickle("\x80\x02G\xc0\x12\x00\x00\x00\x00\x00\x00.", 12), nff);
    expect_eq(serialize_pickle(ot), "\x80\x02G?\xf3\x33\x33\x33\x33\x33\x33.");
    expect_eq(serialize_pickle(nff), string("\x80\x02G\xc0\x12\x00\x00\x00\x00\x00\x00.", 12));
    expect_eq(parse_pickle(serialize_pickle(ot)), ot);
    expect_eq(parse_pickle(serialize_pickle(nff)), nff);
    expect_eq(get_python_repr(serialize_pickle(ot)), "1.2");
    expect_eq(get_python_repr(serialize_pickle(nff)), "-4.5");
  }

  // empty lists
  {
    JSONObject list(vector<JSONObject>({}));
    expect_eq(parse_pickle("(l."), list);
    expect_eq(parse_pickle("]."), list);
    expect_eq(parse_pickle("\x80\x02]."), list);
    expect_eq(parse_pickle("(t."), list);
    expect_eq(parse_pickle(")."), list);
    expect_eq(parse_pickle("\x80\x02)."), list);
    expect_eq(serialize_pickle(list), "\x80\x02].");
    expect_eq(parse_pickle(serialize_pickle(list)), list);
    expect_eq(get_python_repr(serialize_pickle(list)), "[]");
  }

  // simple lists: [null, true, 13, 2.5, "lolz"]
  {
    JSONObject list(vector<JSONObject>({JSONObject(), JSONObject(true), JSONObject((int64_t)13), JSONObject(2.5), JSONObject("lolz")}));
    expect_eq(parse_pickle("(NI01\nI13\nF2.5\nS'lolz'\np1\nt."), list);
    expect_eq(parse_pickle("(NI01\nK\rG@\x04\x00\x00\x00\x00\x00\x00U\x04lolzq\x01t.", 27), list);
    expect_eq(parse_pickle("\x80\x02(N\x88K\rG@\x04\x00\x00\x00\x00\x00\x00U\x04lolzq\x01t.", 26), list);
    expect_eq(serialize_pickle(list), string("\x80\x02(N\x88K\rG@\x04\x00\x00\x00\x00\x00\x00U\x04lolzl.", 24));
    expect_eq(parse_pickle(serialize_pickle(list)), list);
    expect_eq(get_python_repr(serialize_pickle(list)), "[None, True, 13, 2.5, \'lolz\']");
  }

  // empty dicts
  {
    JSONObject dict(unordered_map<string, JSONObject>({}));
    expect_eq(parse_pickle("(d."), dict);
    expect_eq(parse_pickle("}."), dict);
    expect_eq(parse_pickle("\x80\x02}."), dict);
    expect_eq(serialize_pickle(dict), "\x80\x02}.");
    expect_eq(parse_pickle(serialize_pickle(dict)), dict);
    expect_eq(get_python_repr(serialize_pickle(dict)), "{}");
  }

  // simple dicts: {"null": null, "true": true, "13": 13, "2.5": 2.5, "lolz": "omg"}
  {
    JSONObject dict(unordered_map<string, JSONObject>({
      {"null", JSONObject()},
      {"true", JSONObject(true)},
      {"13", JSONObject((int64_t)13)},
      {"2.5", JSONObject(2.5)},
      {"lolz", JSONObject("omg")}
    }));
    expect_eq(parse_pickle("(dp1\nS\'lolz\'\np2\nS\'omg\'\np3\nsS\'13\'\np4\nI13\nsS\'null\'\np5\nNsS\'true\'\np6\nI01\nsS\'2.5\'\np7\nF2.5\ns."), dict);
    expect_eq(parse_pickle("}q\x01(U\x04lolzq\x02U\x03omgq\x03U\x02\x31\x33q\x04K\rU\x04nullq\x05NU\x04trueq\x06I01\nU\x03\x32.5q\x07G@\x04\x00\x00\x00\x00\x00\x00u.", 66), dict);
    expect_eq(parse_pickle("\x80\x02}q\x01(U\x04lolzq\x02U\x03omgq\x03U\x02\x31\x33q\x04K\rU\x04nullq\x05NU\x04trueq\x06\x88U\x03\x32.5q\x07G@\x04\x00\x00\x00\x00\x00\x00u.", 65), dict);

    // because dicts are unordered, we can't rely on a static order for the
    // serialized result - just make sure it unserializes to the same value
    expect_eq(parse_pickle(serialize_pickle(dict)), dict);

    expect_eq(get_sorted_dict_python_repr(serialize_pickle(dict)),
        "[('13', 13), ('2.5', 2.5), ('lolz', 'omg'), ('null', None), ('true', True)]");
  }

  // complex structures: {"1": [null, true], "2": [[13, 14], ["s1", "s2"]], "hax": {"derp": []}}
  {
    JSONObject o(unordered_map<string, JSONObject>({
      {"1", JSONObject(vector<JSONObject>({JSONObject(), JSONObject(true)}))},
      {"2", JSONObject(vector<JSONObject>({
        JSONObject(vector<JSONObject>({JSONObject((int64_t)13), JSONObject((int64_t)14)})),
        JSONObject(vector<JSONObject>({JSONObject("s1"), JSONObject("s2")}))}))},
      {"hax", JSONObject(unordered_map<string, JSONObject>({
        {"derp", JSONObject(vector<JSONObject>())}}))}
    }));
    expect_eq(parse_pickle("(dp1\nS\'1\'\n(lp2\nNaI01\nasS\'hax\'\np3\n(dp4\nS\'derp\'\np5\n(lp6\nssS\'2\'\n(lp7\n(lp8\nI13\naI14\naa(lp9\nS\'s1\'\np10\naS\'s2\'\np11\naas."), o);
    expect_eq(parse_pickle("}q\x01(U\x01\x31]q\x02(NI01\neU\x03haxq\x03}q\x04U\x04\x64\x65rpq\x05]sU\x01\x32]q\x06(]q\x07(K\rK\x0e\x65]q\x08(U\x02s1q\tU\x02s2q\neeu."), o);
    expect_eq(parse_pickle("\x80\x02}q\x01(U\x01\x31]q\x02(N\x88\x65U\x03haxq\x03}q\x04U\x04\x64\x65rpq\x05]sU\x01\x32]q\x06(]q\x07(K\rK\x0e\x65]q\x08(U\x02s1q\tU\x02s2q\neeu."), o);
    expect_eq(parse_pickle(serialize_pickle(o)), o);

    expect_eq(get_sorted_dict_python_repr(serialize_pickle(o)),
        "[('1', [None, True]), ('2', [[13, 14], ['s1', 's2']]), ('hax', {'derp': []})]");
  }

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
