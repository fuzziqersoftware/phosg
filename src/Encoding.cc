#include "Encoding.hh"

#include <stdexcept>

using namespace std;

const char* DEFAULT_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char* URLSAFE_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

string base64_encode(const void* vdata, size_t size, const char* alphabet) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);

  if (!alphabet) {
    alphabet = DEFAULT_ALPHABET;
  }

  string ret;

  // encode blocks of 3 bytes first
  size_t end_offset = (size / 3) * 3;
  for (size_t offset = 0; offset < end_offset; offset += 3) {
    // aaaaaabb bbbbcccc ccdddddd
    uint8_t c1 = data[offset];
    uint8_t c2 = data[offset + 1];
    uint8_t c3 = data[offset + 2];
    ret.push_back(alphabet[(c1 >> 2) & 0x3F]);
    ret.push_back(alphabet[((c1 << 4) & 0x30) | ((c2 >> 4) & 0x0F)]);
    ret.push_back(alphabet[((c2 << 2) & 0x3C) | ((c3 >> 6) & 0x03)]);
    ret.push_back(alphabet[c3 & 0x3F]);
  }

  if (size - end_offset == 2) {
    // aaaaaabb bbbbcccc ========
    uint8_t c1 = data[end_offset];
    uint8_t c2 = data[end_offset + 1];
    ret.push_back(alphabet[(c1 >> 2) & 0x3F]);
    ret.push_back(alphabet[((c1 << 4) & 0x30) | ((c2 >> 4) & 0x0F)]);
    ret.push_back(alphabet[((c2 << 2) & 0x3C)]);
    ret.push_back('=');
  } else if (size - end_offset == 1) {
    // aaaaaabb ======== ========
    uint8_t c1 = data[end_offset];
    ret.push_back(alphabet[(c1 >> 2) & 0x3F]);
    ret.push_back(alphabet[((c1 << 4) & 0x30)]);
    ret.push_back('=');
    ret.push_back('=');
  }

  return ret;
}

string base64_encode(const string& data, const char* alphabet) {
  return base64_encode(data.data(), data.size(), alphabet);
}

string base64_decode(const void* vdata, size_t size, const char* alphabet) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);

  if (!alphabet) {
    alphabet = DEFAULT_ALPHABET;
  }

  // the length must be a multiple of 4
  if (size & 3) {
    throw invalid_argument("size must be a multiple of 4 bytes");
  }

  // compute the inverse alphabet for easier decoding
  // TODO: make this not happen every time, at least for the default alphabets
  string inverse_alphabet(0x100, -1);
  for (uint8_t x = 0; x < 0x40; x++) {
    inverse_alphabet[alphabet[x]] = static_cast<char>(x);
  }
  inverse_alphabet['='] = -0x80;

  // decode blocks of 4 bytes first
  string ret;
  size_t end_offset = size & (~3);
  for (size_t offset = 0; offset < end_offset; offset += 4) {
    // aaaaaabb bbbbcccc ccdddddd
    uint8_t c1 = inverse_alphabet[data[offset]];
    uint8_t c2 = inverse_alphabet[data[offset + 1]];
    uint8_t c3 = inverse_alphabet[data[offset + 2]];
    uint8_t c4 = inverse_alphabet[data[offset + 3]];

    // TODO: this is pretty ugly; clean it up
    if (c4 == 0x80) {
      if (offset != end_offset - 4) {
        throw invalid_argument("string contains padding not at the end");
      }
      if (c3 == 0x80) {
        if ((c1 >= 0x40) || (c2 >= 0x40)) {
          throw invalid_argument("string contains non-base64 characters");
        }
        ret.push_back(((c1 << 2) & 0xFC) | ((c2 >> 4) & 0x03));
      } else {
        if ((c1 >= 0x40) || (c2 >= 0x40) || (c2 >= 0x40)) {
          throw invalid_argument("string contains non-base64 characters");
        }
        ret.push_back(((c1 << 2) & 0xFC) | ((c2 >> 4) & 0x03));
        ret.push_back(((c2 << 4) & 0xF0) | ((c3 >> 2) & 0x0F));
      }
    } else {
      if ((c1 >= 0x40) || (c2 >= 0x40) || (c3 >= 0x40) || (c4 >= 0x40)) {
        throw invalid_argument("string contains non-base64 characters");
      }
      ret.push_back((c1 << 2) | ((c2 >> 4) & 0x03));
      ret.push_back((c2 << 4) | ((c3 >> 2) & 0x0F));
      ret.push_back((c3 << 6) | c4);
    }
  }

  return ret;
}

string base64_decode(const string& data, const char* alphabet) {
  return base64_decode(data.data(), data.size(), alphabet);
}

string rot13(const void* vdata, size_t size) {
  const char* data = reinterpret_cast<const char*>(vdata);
  string ret;
  for (size_t x = 0; x < size; x++) {
    char ch = data[x];
    if (((ch >= 'a') && (ch <= 'm')) || ((ch >= 'A') && (ch <= 'M'))) {
      ch += 13;
    } else if (((ch >= 'n') && (ch <= 'z')) || ((ch >= 'N') && (ch <= 'Z'))) {
      ch -= 13;
    }
    ret.push_back(ch);
  }
  return ret;
}
