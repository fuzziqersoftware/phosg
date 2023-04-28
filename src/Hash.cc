#include "Hash.hh"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "Encoding.hh"
#include "Filesystem.hh"

using namespace std;

uint32_t fnv1a32(const void* data, size_t size, uint32_t hash) {
  const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(data);
  const uint8_t* end_ptr = data_ptr + size;

  for (; data_ptr != end_ptr; data_ptr++) {
    hash = (hash ^ static_cast<uint32_t>(*data_ptr)) * 0x01000193;
  }
  return hash;
}

uint32_t fnv1a32(const std::string& data, uint32_t hash) {
  return fnv1a32(data.data(), data.size(), hash);
}

uint64_t fnv1a64(const void* data, size_t size, uint64_t hash) {
  const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(data);
  const uint8_t* end_ptr = data_ptr + size;

  for (; data_ptr != end_ptr; data_ptr++) {
    hash = (hash ^ static_cast<uint64_t>(*data_ptr)) * 0x00000100000001B3;
  }
  return hash;
}

uint64_t fnv1a64(const string& data, uint64_t hash) {
  return fnv1a64(data.data(), data.size(), hash);
}

static void sha1_process_block(const void* block, uint32_t& h0, uint32_t& h1,
    uint32_t& h2, uint32_t& h3, uint32_t& h4) {
  const le_uint32_t* fields = reinterpret_cast<const le_uint32_t*>(block);

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
  for (; block_offset + 0x40 < size; block_offset += 0x40) {
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
  *reinterpret_cast<be_uint64_t*>(&last_blocks[0x40 * blocks_remaining - 8]) = size * 8;
  sha1_process_block(&last_blocks[0], h0, h1, h2, h3, h4);
  if (blocks_remaining > 1) {
    sha1_process_block(&last_blocks[0x40], h0, h1, h2, h3, h4);
  }

  string ret(20, 0);
  *reinterpret_cast<be_uint32_t*>(ret.data() + 0) = h0;
  *reinterpret_cast<be_uint32_t*>(ret.data() + 4) = h1;
  *reinterpret_cast<be_uint32_t*>(ret.data() + 8) = h2;
  *reinterpret_cast<be_uint32_t*>(ret.data() + 12) = h3;
  *reinterpret_cast<be_uint32_t*>(ret.data() + 16) = h4;
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

  // clang-format off
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
  // clang-format on

  // TODO: Even though this is explicitly not optimized, copying the entire
  // input is really bad. We should ideally not do this at all, and at least not
  // do it for the last chunk (or two if the added bytes span a chunk boundary).
  string input(reinterpret_cast<const char*>(data), orig_size);
  {
    input.push_back(0x80);
    size_t num_zero_bytes = ((64 - ((input.size() + 8) & 0x3F)) & 0x3F);
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
      w[x] = *reinterpret_cast<const be_uint32_t*>(&input[offset + (x * 4)]);
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
  // clang-format off
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
  // clang-format on
  const le_uint32_t* fields = reinterpret_cast<const le_uint32_t*>(block);

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
  for (; block_offset + 0x40 < size; block_offset += 0x40) {
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
  *reinterpret_cast<le_uint64_t*>(&last_blocks[0x40 * blocks_remaining - 8]) = size * 8;
  md5_process_block(&last_blocks[0], a0, b0, c0, d0);
  if (blocks_remaining > 1) {
    md5_process_block(&last_blocks[0x40], a0, b0, c0, d0);
  }

  string ret(16, 0);
  *reinterpret_cast<le_uint32_t*>(ret.data() + 0) = a0;
  *reinterpret_cast<le_uint32_t*>(ret.data() + 4) = b0;
  *reinterpret_cast<le_uint32_t*>(ret.data() + 8) = c0;
  *reinterpret_cast<le_uint32_t*>(ret.data() + 12) = d0;
  return ret;
}

string md5(const std::string& data) {
  return md5(data.data(), data.size());
}

// clang-format off
static const uint32_t crc32_table[0x100] = {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
  0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
  0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
  0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
  0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
  0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
  0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
  0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
  0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
  0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
  0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
  0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
  0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
  0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
  0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
  0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
  0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
  0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
  0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
  0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
  0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
  0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};
// clang-format on

uint32_t crc32(const void* vdata, size_t size, uint32_t cs) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);

  cs = ~cs;
  for (size_t offset = 0; offset < size; offset++) {
    uint8_t table_offset = cs ^ data[offset];
    cs = (cs >> 8) ^ crc32_table[table_offset];
  }
  return ~cs;
}
