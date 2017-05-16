#include "Hash.hh"

#include <stdio.h>

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

string get_random_data(size_t size) {
  static FILE* random_file = NULL;
  if (!random_file) {
    random_file = fopen("/dev/urandom", "rb");
  }
  return freadx(random_file, size);
}
