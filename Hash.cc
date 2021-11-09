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

uint64_t fnv1a64(const string& data, uint64_t hash) {
  return fnv1a64(data.data(), data.size(), hash);
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



static inline uint32_t rotate_right(uint32_t x, uint8_t bits) {
  return (x >> bits) | (x << (32 - bits));
}

string sha256(const void* data, size_t orig_size) {
  // This is mostly copied from the pseudocode on the SHA-256 Wikipedia article.
  // It is not optimized.

  uint32_t h[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
  };

  uint32_t k[64] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
  };

  // TODO: Even though this is explicitly not optimized, copying the entire
  // input is really bad. We should ideally not do this at all, and at least not
  // do it for the last chunk (or two if the added bytes span a chunk boundary).
  string input(reinterpret_cast<const char*>(data), orig_size);
  {
    input.push_back(0x80);
    size_t num_zero_bytes = (64 - ((input.size() + 8) & 0x3F) & 0x3F);
    input.resize(input.size() + num_zero_bytes, '\0');
    uint64_t size_be = bswap64(orig_size << 3);
    input.append(reinterpret_cast<const char*>(&size_be), 8);
    if (input.size() & 0x3F) {
      throw logic_error("padding did not result in correct length");
    }
  }

  for (size_t offset = 0; offset < input.size(); offset += 0x40) {
    uint32_t w[64];
    for (size_t x = 0; x < 16; x++) {
      w[x] = bswap32(*reinterpret_cast<const uint32_t*>(&input[offset + (x * 4)]));
    }

    for (size_t x = 16; x < 64; x++) {
      uint32_t s0 = rotate_right(w[x - 15], 7) ^ rotate_right(w[x - 15], 18) ^ (w[x - 15] >> 3);
      uint32_t s1 = rotate_right(w[x - 2], 17) ^ rotate_right(w[x - 2], 19) ^ (w[x - 2] >> 10);
      w[x] = w[x - 16] + s0 + w[x - 7] + s1;
    }

    uint32_t z[8];
    memcpy(z, h, 0x20);

    for (size_t x = 0; x < 64; x++) {
      uint32_t s1 = rotate_right(z[4], 6) ^ rotate_right(z[4], 11) ^ rotate_right(z[4], 25);
      uint32_t ch = (z[4] & z[5]) ^ ((~z[4]) & z[6]);
      uint32_t temp1 = z[7] + s1 + ch + k[x] + w[x];
      uint32_t s0 = rotate_right(z[0], 2) ^ rotate_right(z[0], 13) ^ rotate_right(z[0], 22);
      uint32_t maj = (z[0] & z[1]) ^ (z[0] & z[2]) ^ (z[1] & z[2]);
      uint32_t temp2 = s0 + maj;
      z[7] = z[6];
      z[6] = z[5];
      z[5] = z[4];
      z[4] = z[3] + temp1;
      z[3] = z[2];
      z[2] = z[1];
      z[1] = z[0];
      z[0] = temp1 + temp2;
    }

    for (size_t x = 0; x < 8; x++) {
      h[x] += z[x];
    }
  }

  for (size_t x = 0; x < 8; x++) {
    h[x] = bswap32(h[x]);
  }
  return string(reinterpret_cast<const char*>(h), 0x20);
}

string sha256(const string& data) {
  return sha256(data.data(), data.size());
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
