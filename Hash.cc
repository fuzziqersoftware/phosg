#include "Hash.hh"

#include <stdio.h>
#include <string.h>

#include <string>

#include "Encoding.hh"
#include "Filesystem.hh"

using namespace std;


uint64_t fnv1a64(const void* data, size_t size, uint64_t hash) {
  const uint8_t *data_ptr = (const uint8_t*)data;
  const uint8_t *end_ptr = data_ptr + size;

  for (; data_ptr != end_ptr; data_ptr++) {
    hash = (hash ^ (uint64_t)*data_ptr) * 0x00000100000001B3;
  }
  return hash;
}

uint64_t fnv1a64(const string& data) {
  return fnv1a64(data.data(), data.size());
}

static void sha1_process_block(const void* block, uint32_t& h0, uint32_t& h1,
    uint32_t& h2, uint32_t& h3, uint32_t& h4) {
  const uint32_t* fields = reinterpret_cast<const uint32_t*>(block);

  uint32_t extended_fields[80];
  for (size_t x = 0; x < 16; x++) {
    extended_fields[x] = fields[x];
  }
  for (size_t x = 16; x < 80; x++) {
    uint32_t z = bswap32(extended_fields[x - 3] ^ extended_fields[x - 8] ^
                         extended_fields[x - 14] ^ extended_fields[x - 16]);
    extended_fields[x] = bswap32((z << 1) | ((z >> 31) & 1));
  }

  uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
  for (size_t x = 0; x < 80; x++) {
    uint32_t f, k;
    if (x < 20) {
      f = (b & c) | ((~b) & d);
      k = 0x5A827999;
    } else if (x < 40) {
      f = b ^ c ^ d;
      k = 0x6ED9EBA1;
    } else if (x < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8F1BBCDC;
    } else {
      f = b ^ c ^ d;
      k = 0xCA62C1D6;
    }

    uint32_t new_a = ((a << 5) | ((a >> 27) & 0x1F)) + f + e + k + bswap32(extended_fields[x]);
    e = d;
    d = c;
    c = (b << 30) | ((b >> 2) & 0x3FFFFFFF);
    b = a;
    a = new_a;
  }

  h0 += a;
  h1 += b; 
  h2 += c;
  h3 += d;
  h4 += e;
}

string sha1(const void* data, size_t size) {
  // TODO: this is broken somehow; fix it
  uint32_t h0 = 0x67452301;
  uint32_t h1 = 0xEFCDAB89;
  uint32_t h2 = 0x98BADCFE;
  uint32_t h3 = 0x10325476;
  uint32_t h4 = 0xC3D2E1F0;

  // process blocks from the message exactly as they are
  size_t block_offset = 0;
  for (; block_offset + 0x40 < size; block_offset += 0x40){
    sha1_process_block(reinterpret_cast<const uint8_t*>(data) + block_offset,
        h0, h1, h2, h3, h4);
  }

  // we're going to append at least 9 bytes to the end. first handle the case
  // where doing so creates two new blocks.
  size_t remaining_bytes = size - block_offset;
  uint8_t last_blocks[0x80];
  memcpy(last_blocks, reinterpret_cast<const uint8_t*>(data) + block_offset,
      remaining_bytes);
  last_blocks[remaining_bytes] = 0x80;

  size_t blocks_remaining = 1 + (remaining_bytes > (0x40 - 9));
  for (remaining_bytes++; remaining_bytes < (0x40 * blocks_remaining - 8);
      remaining_bytes++) {
    last_blocks[remaining_bytes] = 0;
  }
  *reinterpret_cast<uint64_t*>(&last_blocks[0x40 * blocks_remaining - 8]) = bswap64(size * 8);
  sha1_process_block(&last_blocks[0], h0, h1, h2, h3, h4);
  if (blocks_remaining > 1) {
    sha1_process_block(&last_blocks[0x40], h0, h1, h2, h3, h4);
  }

  string ret(20, 0);
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 0)) = bswap32(h0);
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 4)) = bswap32(h1);
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 8)) = bswap32(h2);
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 12)) = bswap32(h3);
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 16)) = bswap32(h4);
  return ret;
}

string sha1(const string& data) {
  return sha1(data.data(), data.size());
}

static void md5_process_block(const void* block, uint32_t& a0, uint32_t& b0,
    uint32_t& c0, uint32_t& d0) {
  static const uint32_t shifts[64] = {
      7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
      5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
      4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
      6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21};
  static const uint32_t sine_table[64] = {
      0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
      0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
      0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
      0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
      0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
      0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
      0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
      0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
      0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
      0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
      0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
      0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
      0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
      0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
      0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
      0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391};
  const uint32_t* fields = reinterpret_cast<const uint32_t*>(block);

  uint32_t a = a0, b = b0, c = c0, d = d0;
  for (size_t x = 0; x < 64; x++) {
    uint32_t f, g;
    if (x < 16) {
      f = (b & c) | ((~b) & d);
      g = x;
    } else if (x < 32) {
      f = (b & d) | (c & (~d));
      g = ((5 * x) + 1) & 15;
    } else if (x < 48) {
      f = b ^ c ^ d;
      g = ((3 * x) + 5) & 15;
    } else {
      f = c ^ (b | (~d));
      g = (7 * x) & 15;
    }
    uint32_t dt = d;
    uint32_t b_addend = a + f + sine_table[x] + fields[g];
    d = c;
    c = b;
    b = b + ((b_addend << shifts[x]) | (b_addend >> (32 - shifts[x])));
    a = dt;
  }
  a0 += a;
  b0 += b;
  c0 += c;
  d0 += d;
}

string md5(const void* data, size_t size) {
  uint32_t a0 = 0x67452301;
  uint32_t b0 = 0xEFCDAB89;
  uint32_t c0 = 0x98BADCFE;
  uint32_t d0 = 0x10325476;

  // process blocks from the message exactly as they are
  size_t block_offset = 0;
  for (; block_offset + 0x40 < size; block_offset += 0x40){
    md5_process_block(reinterpret_cast<const uint8_t*>(data) + block_offset,
        a0, b0, c0, d0);
  }

  // we're going to append at least 9 bytes to the end. first handle the case
  // where doing so creates two new blocks.
  size_t remaining_bytes = size - block_offset;
  uint8_t last_blocks[0x80];
  memcpy(last_blocks, reinterpret_cast<const uint8_t*>(data) + block_offset,
      remaining_bytes);
  last_blocks[remaining_bytes] = 0x80;

  size_t blocks_remaining = 1 + (remaining_bytes > (0x40 - 9));
  for (remaining_bytes++; remaining_bytes < (0x40 * blocks_remaining - 8);
      remaining_bytes++) {
    last_blocks[remaining_bytes] = 0;
  }
  *reinterpret_cast<uint64_t*>(&last_blocks[0x40 * blocks_remaining - 8]) = size * 8;
  md5_process_block(&last_blocks[0], a0, b0, c0, d0);
  if (blocks_remaining > 1) {
    md5_process_block(&last_blocks[0x40], a0, b0, c0, d0);
  }

  string ret(16, 0);
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 0)) = a0;
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 4)) = b0;
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 8)) = c0;
  *reinterpret_cast<uint32_t*>(const_cast<char*>(ret.data() + 12)) = d0;
  return ret;
}

string md5(const std::string& data) {
  return md5(data.data(), data.size());
}

string get_random_data(size_t size) {
  static FILE* random_file = NULL;
  if (!random_file) {
    random_file = fopen("/dev/urandom", "rb");
  }
  return freadx(random_file, size);
}
