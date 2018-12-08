#include "JSONPickle.hh"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <unordered_map>

#include "Encoding.hh"
#include "Strings.hh"

using namespace std;


shared_ptr<JSONObject> parse_pickle(const std::string& pickle_data) {
  return parse_pickle(pickle_data.data(), pickle_data.size());
}

shared_ptr<JSONObject> parse_pickle(const void* pickle_data) {
  return parse_pickle(pickle_data, strlen((const char*)pickle_data));
}

shared_ptr<JSONObject> parse_pickle(const void* data, size_t size) {
  const char* buffer = (const char*)data;
  size_t offset = 0;

  vector<size_t> mark_stk;
  vector<shared_ptr<JSONObject>> stk;
  unordered_map<int64_t, shared_ptr<JSONObject>> memo;
  while (offset < size) {
    char cmd = buffer[offset];
    offset++;

    switch (cmd) {
      case '(':    // MARK            - push special markobject on stack
        mark_stk.emplace_back(stk.size());
        break;

      case '.':    // STOP            - every pickle ends with STOP
        if (stk.size() != 1) {
          throw JSONObject::parse_error("stop command with incorrect stack size");
        }
        if (offset != size) {
          throw JSONObject::parse_error("extra data after pickled object");
        }
        return stk.back();

      case '0':    // POP             - discard topmost stack item
        if (stk.empty() || mark_stk.empty()) {
          throw JSONObject::parse_error("pop command with empty stacks");
        }

        if (!mark_stk.empty() && (mark_stk.back() == stk.size())) {
          mark_stk.pop_back();
        } else {
          stk.pop_back();
        }
        break;

      case '1':    // POP_MARK        - discard stack top through topmost markobject
        if (mark_stk.empty()) {
          throw JSONObject::parse_error("pop mark command with no marks");
        }
        while (stk.size() > mark_stk.back()) {
          stk.pop_back();
        }
        mark_stk.pop_back();
        break;

      case '2':    // DUP             - duplicate top stack item
        if (stk.empty()) {
          throw JSONObject::parse_error("duplicate command with empty stack");
        }
        stk.emplace_back(stk.back());
        break;

      case 'F': {  // FLOAT           - push float object; decimal string argument
        char* endptr;
        stk.emplace_back(new JSONObject(strtod(&buffer[offset], &endptr)));
        if (endptr == &buffer[offset]) {
          throw JSONObject::parse_error("blank float value");
        }
        offset += (endptr - &buffer[offset]);
        if (buffer[offset] != '\n') {
          throw JSONObject::parse_error("incorrect string terminator");
        }
        offset++;
        break;
      }

      case 'I': {  // INT             - push integer or bool; decimal string argument
        char* endptr;
        int64_t value = strtoll(&buffer[offset], &endptr, 10);
        if (endptr == &buffer[offset]) {
          throw JSONObject::parse_error("blank integer value");
        }

        // special cases: "00" = False, "01" = True
        if (((value & 1) == value) && ((endptr - &buffer[offset]) == 2)) {
          stk.emplace_back(new JSONObject((bool)value));
        } else {
          stk.emplace_back(new JSONObject(value));
        }

        offset += (endptr - &buffer[offset]);
        if (buffer[offset] != '\n') {
          throw JSONObject::parse_error("incorrect int string terminator");
        }
        offset++;
        break;
      }

      case 'J':    // BININT          - push four-byte signed int
        if (size - offset < 4) {
          throw JSONObject::parse_error("no space for 4-byte binary int");
        }
        stk.emplace_back(new JSONObject((int64_t)(*(int32_t*)&buffer[offset])));
        offset += 4;
        break;

      case 'K':    // BININT1         - push 1-byte unsigned int
        if (size - offset < 1) {
          throw JSONObject::parse_error("no space for 1-byte binary int");
        }
        stk.emplace_back(new JSONObject((int64_t)((uint8_t)buffer[offset])));
        offset++;
        break;

      case 'L':    // LONG            - push long; decimal string argument
        char* endptr;
        stk.emplace_back(new JSONObject((int64_t)strtoll(&buffer[offset],
            &endptr, 10)));
        if (endptr == &buffer[offset]) {
          throw JSONObject::parse_error("blank long value");
        }
        offset += (endptr - &buffer[offset]);
        if (buffer[offset] != 'L') {
          throw JSONObject::parse_error("incorrect long terminator");
        }
        offset++;
        if (buffer[offset] != '\n') {
          throw JSONObject::parse_error("incorrect long string terminator");
        }
        offset++;
        break;

      case 'M':    // BININT2         - push 2-byte unsigned int
        if (size - offset < 2) {
          throw JSONObject::parse_error("no space for 2-byte binary int");
        }
        stk.emplace_back(new JSONObject(
            (int64_t)(*(uint16_t*)&buffer[offset])));
        offset += 2;
        break;

      case 'N':    // NONE            - push None
        stk.emplace_back(new JSONObject());
        break;

      case 'V': {  // UNICODE         - push Unicode string; raw-unicode-escaped'd argument
        // expect V<data>\n
        string s;
        while (offset < size && buffer[offset] != '\n') {
          if (buffer[offset] == '\\' && (offset < size - 1)) {
            offset++;
            if (offset >= size) {
              throw JSONObject::parse_error("end of string literal during escape sequence");
            }
            if (buffer[offset] == 'u') {
              if (offset > size - 5) {
                throw JSONObject::parse_error("end of string literal during unicode escape sequence");
              }
              int16_t ch = ((value_for_hex_char(buffer[offset + 1]) << 12) |
                            (value_for_hex_char(buffer[offset + 2]) << 8) |
                            (value_for_hex_char(buffer[offset + 3]) << 4) |
                             value_for_hex_char(buffer[offset + 4]));
              if (ch & 0xFF00) {
                throw JSONObject::parse_error("unknown unicode escape sequence");
              }
              s += (char)ch;
              offset += 4;
            } else if (buffer[offset] == 'x') {
              if (offset > size - 3) {
                throw JSONObject::parse_error("end of string literal during hex escape sequence");
              }
              s += ((value_for_hex_char(buffer[offset + 1]) << 4) |
                     value_for_hex_char(buffer[offset + 2]));
              offset += 2;
            } else if (buffer[offset] == 't') {
              s += '\t';
            } else if (buffer[offset] == 'n') {
              s += '\n';
            } else if (buffer[offset] == 'r') {
              s += '\r';
            } else {
              s += buffer[offset];
            }
          } else {
            s += buffer[offset];
          }
          offset++;
        }

        if (offset >= size) {
          throw JSONObject::parse_error("unterminated string");
        }
        offset++; // skip the \n

        stk.emplace_back(new JSONObject(move(s)));
        break;
      }

      case 'S': {  // STRING          - push string; NL-terminated string argument
        // expect S'<data>'\n or S"<data>"\n"
        char sentinel = buffer[offset];
        if ((sentinel != '\'') && (sentinel != '\"')) {
          throw JSONObject::parse_error("incorrect string start sentinel");
        }
        offset++;

        string s;
        while (offset < size && buffer[offset] != sentinel) {
          if (buffer[offset] == '\\' && (offset < size - 1)) {
            offset++;
            if (offset >= size) {
              throw JSONObject::parse_error("end of string literal during escape sequence");
            }
            if (buffer[offset] == 'x') {
              if (offset > size - 3) {
                throw JSONObject::parse_error("end of string literal during hex escape sequence");
              }
              s += ((value_for_hex_char(buffer[offset + 1]) << 4) |
                     value_for_hex_char(buffer[offset + 2]));
              offset += 2;
            } else if (buffer[offset] == 't') {
              s += '\t';
            } else if (buffer[offset] == 'n') {
              s += '\n';
            } else if (buffer[offset] == 'r') {
              s += '\r';
            } else {
              s += buffer[offset];
            }
          } else {
            s += buffer[offset];
          }
          offset++;
        }

        if (offset >= size) {
          throw JSONObject::parse_error("unterminated string");
        }
        if (buffer[offset] != sentinel) {
          throw JSONObject::parse_error("incorrect string end sentinel");
        }
        offset++;
        if (buffer[offset] != '\n') {
          throw JSONObject::parse_error("incorrect string terminator");
        }
        offset++;

        stk.emplace_back(new JSONObject(move(s)));
        break;
      }

      case '\x8D': // BINUNICODE8     - push very long string
      case '\x8E': { // BINBYTES8       - push very long bytes string
        if (size - offset < 8) {
          throw JSONObject::parse_error("no space for 8-byte string length");
        }

        size_t length = *(uint64_t*)(&buffer[offset]);
        if (offset + length > size) {
          throw JSONObject::parse_error("string length overflows buffer");
        }
        offset += 8;

        stk.emplace_back(new JSONObject(&buffer[offset], length));
        offset += length;
        break;
      }

      case 'X':    // BINUNICODE      -   "     "       "  ; counted UTF-8 string argument
      case 'B':    // BINBYTES        - push bytes; counted binary string argument
      case 'T': {  // BINSTRING       - push string; counted binary string argument
        if (size - offset < 4) {
          throw JSONObject::parse_error("no space for 4-byte string length");
        }

        size_t length = *(uint32_t*)(&buffer[offset]);
        if (offset + length > size) {
          throw JSONObject::parse_error("string length overflows buffer");
        }
        offset += 4;

        stk.emplace_back(new JSONObject(&buffer[offset], length));
        offset += length;
        break;
      }

      case '\x8C': // SHORT_BINUNICODE - push short string; UTF-8 length < 256 bytes
      case 'C':    // SHORT_BINBYTES  -  "     "   ;    "      "       "      " < 256 bytes
      case 'U': {  // SHORT_BINSTRING -  "     "   ;    "      "       "      " < 256 bytes
        if (size - offset < 1) {
          throw JSONObject::parse_error("no space for 1-byte string length");
        }

        size_t length = (uint8_t)buffer[offset];
        if (offset + length > size) {
          throw JSONObject::parse_error("string length overflows buffer");
        }
        offset++;

        stk.emplace_back(new JSONObject(&buffer[offset], length));
        offset += length;
        break;
      }

      case 'a': {  // APPEND          - append stack top to list below it
        if (stk.size() < 2) {
          throw JSONObject::parse_error("not enough items on stack for append");
        }
        shared_ptr<JSONObject> j = move(stk.back());
        stk.pop_back();
        stk.back()->as_list().emplace_back(move(j));
        break;
      }

      case 'd': {  // DICT            - build a dict from stack items
        if (mark_stk.empty()) {
          throw JSONObject::parse_error("dict construct operation with no mark");
        }

        size_t mark = mark_stk.back();
        if ((stk.size() - mark) & 1) {
          throw JSONObject::parse_error("dict contains unequal numbers of keys and values");
        }
        mark_stk.pop_back();

        unordered_map<string, shared_ptr<JSONObject>> d;
        while (stk.size() > mark) {
          shared_ptr<JSONObject> value = move(stk.back());
          stk.pop_back();
          if (!stk.back()->is_string()) {
            throw JSONObject::parse_error("dict contains keys that aren\'t strings");
          }
          d.emplace(stk.back()->as_string(), move(value));
          stk.pop_back();
        }
        stk.emplace_back(new JSONObject(move(d)));
        break;
      }

      case '}':    // EMPTY_DICT      - push empty dict
        stk.emplace_back(new JSONObject(
            unordered_map<string, shared_ptr<JSONObject>>()));
        break;

      case '\x90': // ADDITEMS        - modify set by adding topmost stack items
      case 'e': {  // APPENDS         - extend list on stack by topmost stack slice
        if (mark_stk.empty() || stk.empty()) {
          throw JSONObject::parse_error("stack empty on list append");
        }

        size_t mark = mark_stk.back();
        if (mark <= 0) {
          throw JSONObject::parse_error("no item before mark");
        }
        mark_stk.pop_back();

        shared_ptr<JSONObject>& list = stk[mark - 1];
        if (!list->is_list()) {
          throw JSONObject::parse_error("append to non-list");
        }
        auto& l = list->as_list();

        for (size_t x = mark; x < stk.size(); x++) {
          l.emplace_back(stk[x]);
        }
        stk.resize(mark);
        break;
      }

      case '\x91': // FROZENSET       - build frozenset from topmost stack items
      case 't':    // TUPLE           - build tuple from topmost stack items
      case 'l': {  // LIST            - build list from topmost stack items
        if (mark_stk.empty()) {
          throw JSONObject::parse_error("stack empty on list append");
        }

        size_t mark = mark_stk.back();
        mark_stk.pop_back();

        vector<shared_ptr<JSONObject>> l;
        for (size_t x = mark; x < stk.size(); x++) {
          l.emplace_back(move(stk[x]));
        }
        stk.resize(mark);
        stk.emplace_back(new JSONObject(l));
        break;
      }

      case '\x8F': // EMPTY_SET       - push empty set
      case ')':    // EMPTY_TUPLE     - push empty tuple
      case ']':    // EMPTY_LIST      - push empty list
        stk.emplace_back(new JSONObject(vector<shared_ptr<JSONObject>>()));
        break;

      case 's': {  // SETITEM         - add key+value pair to dict
        if (stk.size() < 3) {
          throw JSONObject::parse_error("stack empty on dict setitem");
        }

        shared_ptr<JSONObject>& d = stk[stk.size() - 3];
        shared_ptr<JSONObject>& k = stk[stk.size() - 2];
        shared_ptr<JSONObject>& v = stk[stk.size() - 1];
        if (!d->is_dict()) {
          throw JSONObject::parse_error("setitem on non-dict");
        }
        if (!k->is_string()) {
          throw JSONObject::parse_error("setitem with non-string key");
        }
        d->as_dict().emplace(k->as_string(), move(v));
        stk.pop_back();
        stk.pop_back();
        break;
      }

      case 'u': {  // SETITEMS        - modify dict by adding topmost key+value pairs
        if (mark_stk.empty()) {
          throw JSONObject::parse_error("setitems with no mark");
        }

        size_t mark = mark_stk.back();
        if ((stk.size() - mark) & 1) {
          throw JSONObject::parse_error("setitems with unequal numbers of keys and values");
        }
        if (mark < 1) {
          throw JSONObject::parse_error("setitems with no dict");
        }
        if (!stk[mark - 1]->is_dict()) {
          throw JSONObject::parse_error("setitems on non-dict");
        }
        mark_stk.pop_back();
        auto& d = stk[mark - 1]->as_dict();

        while (!stk.empty() && stk.size() > mark) {
          shared_ptr<JSONObject> value = move(stk.back());
          stk.pop_back();
          if (!stk.back()->is_string()) {
            throw JSONObject::parse_error("setitems with non-string key");
          }
          d.emplace(stk.back()->as_string(), move(value));
          stk.pop_back();
        }
        break;
      }

      case 'G':    // BINFLOAT        - push float; arg is 8-byte float encoding
        // note: apparently this is big-endian
        if (size - offset < 8) {
          throw JSONObject::parse_error("no space for binary double");
        }
        stk.emplace_back(new JSONObject(bswap64f(*(uint64_t*)&buffer[offset])));
        offset += 8;
        break;

      case '\x80': // PROTO           - identify pickle protocol
        if (offset != 1) {
          throw JSONObject::parse_error("protocol change in midstream");
        }
        if (size - offset < 1) {
          throw JSONObject::parse_error("protocol opcode with no argument");
        }
        // we complain about it if it's missing, then ignore it. life is hard
        offset++;
        break;

      case '\x85': { // TUPLE1          - build 1-tuple from stack top
        if (stk.size() < 1) {
          throw JSONObject::parse_error("stack empty on 1-tuple construction");
        }
        vector<shared_ptr<JSONObject>> tup;
        tup.emplace_back(move(stk.back()));
        stk.pop_back();
        stk.emplace_back(new JSONObject(move(tup)));
        break;
      }

      case '\x86': { // TUPLE2          - build 2-tuple from two topmost stack items
        if (stk.size() < 2) {
          throw JSONObject::parse_error("stack empty on 2-tuple construction");
        }
        vector<shared_ptr<JSONObject>> tup;
        tup.emplace_back(move(stk[stk.size() - 2]));
        tup.emplace_back(move(stk[stk.size() - 1]));
        stk.resize(stk.size() - 2);
        stk.emplace_back(new JSONObject(move(tup)));
        break;
      }

      case '\x87': { // TUPLE3          - build 3-tuple from three topmost stack items
        if (stk.size() < 3) {
          throw JSONObject::parse_error("stack empty on 3-tuple construction");
        }
        vector<shared_ptr<JSONObject>> tup;
        tup.emplace_back(move(stk[stk.size() - 3]));
        tup.emplace_back(move(stk[stk.size() - 2]));
        tup.emplace_back(move(stk[stk.size() - 1]));
        stk.resize(stk.size() - 3);
        stk.emplace_back(new JSONObject(move(tup)));
        break;
      }

      case '\x88': // NEWTRUE         - push True
        stk.emplace_back(new JSONObject(true));
        break;

      case '\x89': // NEWFALSE        - push False
        stk.emplace_back(new JSONObject(false));
        break;

      case '\x8a': // LONG1           - push long from < 256 bytes
      case '\x8b': { // LONG4           - push really big long
        uint32_t num_bytes;

        if (cmd == '\x8a') {
          if (size - offset < 1) {
            throw JSONObject::parse_error("no space for 1-byte long opcode size");
          }
          num_bytes = static_cast<uint8_t>(buffer[offset]);
          offset++;

        } else {
          if (size - offset < 4) {
            throw JSONObject::parse_error("no space for 4-byte long opcode size");
          }
          num_bytes = *reinterpret_cast<const uint32_t*>(&buffer[offset]);
          offset += 4;
        }

        if (num_bytes > 8) {
          throw JSONObject::parse_error("long integer does not fit in 64 bits");
        }
        if (size - offset < num_bytes) {
          throw JSONObject::parse_error("long opcode exceeds end of data");
        }

        int64_t value = 0;
        uint64_t byte = 0;
        for (size_t x = 0; x < num_bytes; x++) {
          byte = static_cast<uint8_t>(buffer[offset]);
          offset++;
          value |= (byte << (x << 3));
        }

        // sign-extend from the last byte if necessary
        if ((num_bytes < 8) && (byte & 0x80)) {
          value |= ~((1LL << (num_bytes << 3)) - 1);
        }

        stk.emplace_back(new JSONObject(value));
        break;
      }

      case '\x94': // MEMOIZE         - store top of the stack in memo
        memo[memo.size()] = stk.back();
        break;

      case 'p': {  // PUT             - store stack top in memo; index is string arg
        char* endptr;
        int64_t index = strtoll(&buffer[offset], &endptr, 10);
        if (endptr == &buffer[offset]) {
          throw JSONObject::parse_error("blank integer value");
        }

        offset += (endptr - &buffer[offset]);
        if (buffer[offset] != '\n') {
          throw JSONObject::parse_error("incorrect put string terminator");
        }
        offset++;

        if (stk.empty()) {
          throw JSONObject::parse_error("put opcode with empty stack");
        }
        memo.emplace(index, stk.back());
        break;
      }
      case 'q': {  // BINPUT          -   "     "    "   "   " ;   "    " 1-byte arg
        if (size - offset < 1) {
          throw JSONObject::parse_error("no space for 1-byte put opcode index");
        }
        int64_t index = static_cast<uint8_t>(buffer[offset]);
        offset++;

        if (stk.empty()) {
          throw JSONObject::parse_error("put opcode with empty stack");
        }
        memo.emplace(index, stk.back());
        break;
      }
      case 'r': {  // LONG_BINPUT     -   "     "    "   "   " ;   "    " 4-byte arg
        if (size - offset < 4) {
          throw JSONObject::parse_error("no space for 4-byte put opcode index");
        }
        int64_t index = *reinterpret_cast<const uint32_t*>(&buffer[offset]);
        offset += 4;

        if (stk.empty()) {
          throw JSONObject::parse_error("put opcode with empty stack");
        }
        memo.emplace(index, stk.back());
        break;
      }

      // framing isn't supported here; the data is already in memory so there's
      // no need to deal with frames
      case '\x95': // FRAME           - indicate the beginning of a new frame
        offset += 8;
        break;

      case 'g': {  // GET             - push item from memo on stack; index is string arg
        char* endptr;
        int64_t index = strtoll(&buffer[offset], &endptr, 10);
        if (endptr == &buffer[offset]) {
          throw JSONObject::parse_error("blank integer value");
        }

        offset += (endptr - &buffer[offset]);
        if (buffer[offset] != '\n') {
          throw JSONObject::parse_error("incorrect get string terminator");
        }
        offset++;

        try {
          stk.emplace_back(memo.at(index));
        } catch (const out_of_range&) {
          throw JSONObject::parse_error("memo get refers to missing index");
        }
        break;
      }
      case 'h': {  // BINGET          -   "    "    "    "   "   "  ;   "    " 1-byte arg
        if (size - offset < 1) {
          throw JSONObject::parse_error("no space for 1-byte get opcode index");
        }
        int64_t index = static_cast<uint8_t>(buffer[offset]);
        offset++;

        try {
          stk.emplace_back(memo.at(index));
        } catch (const out_of_range&) {
          throw JSONObject::parse_error("memo get refers to missing index");
        }
        break;
      }
      case 'j': {  // LONG_BINGET     - push item from memo on stack; index is 4-byte arg
        if (size - offset < 4) {
          throw JSONObject::parse_error("no space for 4-byte get opcode index");
        }
        int64_t index = *reinterpret_cast<const uint32_t*>(&buffer[offset]);
        offset += 4;

        try {
          stk.emplace_back(memo.at(index));
        } catch (const out_of_range&) {
          throw JSONObject::parse_error("memo get refers to missing index");
        }
        break;
      }

      case 'P':    // PERSID          - push persistent object; id is taken from string arg
      case 'Q':    // BINPERSID       -  "       "         "  ;  "  "   "     "  stack
      case 'R':    // REDUCE          - apply callable to argtuple, both on stack
      case 'b':    // BUILD           - call __setstate__ or __dict__.update()
      case 'c':    // GLOBAL          - push self.find_class(modname, name); 2 string args
      case 'i':    // INST            - build & push class instance
      case 'o':    // OBJ             - build & push class instance
      case '\x81': // NEWOBJ          - build object by applying cls.__new__ to argtuple
      case '\x82': // EXT1            - push object from extension registry; 1-byte index
      case '\x83': // EXT2            - ditto, but 2-byte index
      case '\x84': // EXT4            - ditto, but 4-byte index
      case '\x92': // NEWOBJ_EX       - like NEWOBJ but work with keyword only arguments
      case '\x93': // STACK_GLOBAL    - same as GLOBAL but using names on the stacks
        throw JSONObject::parse_error("pickled python objects not supported");
    }

    if (!mark_stk.empty() && (mark_stk.back() > stk.size())) {
      throw JSONObject::parse_error("mark object was used incorrectly");
    }
  }

  throw JSONObject::parse_error("completed parsing with no return command");
}

void serialize_pickle_recursive(string& buffer, const JSONObject& o) {
  if (o.is_null()) {
    buffer += 'N';

  } else if (o.is_bool()) {
    buffer += (o.as_bool() ? '\x88' : '\x89');

  } else if (o.is_string()) {
    const string& s = o.as_string();
    size_t length = s.size();
    if (length > 0xFFFFFFFF) {
      throw JSONObject::parse_error("string too long to be serialized in pickle format");
    } else if (length > 0xFF) {
      uint32_t length32 = length;
      buffer += 'T';
      buffer.append((char*)&length32, sizeof(length32));
    } else {
      uint8_t length8 = length;
      buffer += 'U';
      buffer.append((char*)&length8, sizeof(length8));
    }
    buffer += s;

  } else if (o.is_int()) {
    int64_t v = o.as_int();
    if (v >= 0 && v < 0x100) {
      uint8_t v8 = v;
      buffer += 'K';
      buffer.append((char*)&v8, sizeof(v8));

    } else if (v >= 0 && v < 0x10000) {
      uint16_t v16 = v;
      buffer += 'M';
      buffer.append((char*)&v16, sizeof(v16));

    } else if (((v & 0xFFFFFFFF80000000) == 0) || ((v & 0xFFFFFFFF80000000) == 0xFFFFFFFF80000000)) {
      int32_t v32 = v;
      buffer += 'J';
      buffer.append((char*)&v32, sizeof(v32));

    } else {
      // this library only supports ints up to 64 bits, so we don't need to
      // think about using LONG4 here
      buffer += '\x8A'; // LONG1

      size_t size_offset = buffer.size();
      buffer += '\x00';

      // note: we don't have to think about v being 0 or -1 (which would produce
      // incorrect results here) because 0 serializes using K and -1 serializes
      // using J above
      if (v > 0) {
        while (v) {
          buffer += static_cast<char>(v & 0xFF);
          v >>= 8;
          buffer[size_offset]++;
        }

      } else { // v < 0
        while (v != -1) {
          buffer += static_cast<char>(v & 0xFF);
          v >>= 8;
          buffer[size_offset]++;
        }
      }
    }

  } else if (o.is_float()) {
    uint64_t v = bswap64f(o.as_float());
    buffer += "G";
    buffer.append((char*)&v, sizeof(v));

  } else if (o.is_list()) {
    const auto& l = o.as_list();
    if (l.empty()) {
      buffer += "]";
    } else {
      buffer += "("; // mark
      for (const auto& i : o.as_list()) {
        serialize_pickle_recursive(buffer, *i);
      }
      buffer += "l";
    }

  } else if (o.is_dict()) {
    const auto& d = o.as_dict();
    if (d.empty()) {
      buffer += "}";
    } else {
      buffer += "("; // mark
      for (const auto& i : o.as_dict()) {
        serialize_pickle_recursive(buffer, JSONObject(i.first));
        serialize_pickle_recursive(buffer, *i.second);
      }
      buffer += "d";
    }

  } else {
    throw JSONObject::parse_error("type is not pickleable");
  }
}

string serialize_pickle(const JSONObject& o) {
  string buffer = "\x80\x02";
  serialize_pickle_recursive(buffer, o);
  buffer += ".";
  return buffer;
}
