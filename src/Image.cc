#include "Image.hh"

#define __STDC_FORMAT_MACROS
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <stdexcept>

#include <zlib.h>

#include "Filesystem.hh"
#include "Strings.hh"

#include "Platform.hh"

#include "ImageTextFont.hh"

using namespace std;

Image::unknown_format::unknown_format(const std::string& what) : runtime_error(what) {}

struct ExpandedColor {
  uint64_t r;
  uint64_t g;
  uint64_t b;
  uint64_t a;
};

static ExpandedColor expand_color(uint32_t c) {
  return {(c >> 24) & 0xFF, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF};
}

static uint32_t compress_color(uint64_t r, uint64_t g, uint64_t b, uint64_t a) {
  return ((r & 0xFF) << 24) | ((g & 0xFF) << 16) | ((b & 0xFF) << 8) | (a & 0xFF);
}

struct WindowsBitmapFileHeader {
  le_uint16_t magic;
  le_uint32_t file_size;
  le_uint16_t reserved[2];
  le_uint32_t data_offset;
} __attribute__((packed));

// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapv5header
struct WindowsBitmapInfoHeader {
  le_uint32_t header_size;
  le_int32_t width;
  le_int32_t height;
  le_uint16_t num_planes;
  le_uint16_t bit_depth;
  le_uint32_t compression; // only BI_RGB (0) and BI_BITFIELDS (3) are supported
  le_uint32_t image_size;
  le_int32_t x_pixels_per_meter;
  le_int32_t y_pixels_per_meter;
  le_uint32_t num_used_colors;
  le_uint32_t num_important_colors;

  // V4 header starts here
  le_uint32_t bitmask_r;
  le_uint32_t bitmask_g;
  le_uint32_t bitmask_b;
  le_uint32_t bitmask_a;
  le_uint32_t color_space_type;
  le_uint32_t chromacity_endpoints[9];
  le_uint32_t gamma_r;
  le_uint32_t gamma_g;
  le_uint32_t gamma_b;

  // V5 header starts here
  le_uint32_t render_intent;
  le_uint32_t color_profile_data;
  le_uint32_t color_profile_size;
  le_uint32_t reserved;

  // Size of basic BMP header (before V4)
  static const size_t SIZE24 = 0x28;
} __attribute__((packed));

struct WindowsBitmapHeader {
  WindowsBitmapFileHeader file_header;
  WindowsBitmapInfoHeader info_header;
} __attribute__((packed));

static size_t init_bmp_header(WindowsBitmapHeader& header,
    ssize_t width, ssize_t height, bool has_alpha,
    size_t pixel_bytes, size_t row_padding_bytes) {
  size_t header_size = sizeof(WindowsBitmapFileHeader) + (has_alpha ? sizeof(WindowsBitmapInfoHeader) : WindowsBitmapInfoHeader::SIZE24);

  // Clear header so we don't have initialize fields that are 0
  header = {};
  header.file_header.magic = 0x4D42; // 'BM'
  header.file_header.file_size = header_size + (width * height * pixel_bytes) + (row_padding_bytes * height);
  header.file_header.reserved[0] = 0;
  header.file_header.reserved[1] = 0;
  header.file_header.data_offset = header_size;
  header.info_header.header_size = header_size - sizeof(WindowsBitmapFileHeader);
  header.info_header.width = width;
  header.info_header.height = height;
  header.info_header.num_planes = 1;
  header.info_header.bit_depth = has_alpha ? 32 : 24;
  header.info_header.compression = has_alpha ? 3 : 0; // BI_BITFIELDS or BI_RGB
  header.info_header.image_size = has_alpha ? (width * height * 4) : 0; // 0 acceptable for BI_RGB
  header.info_header.x_pixels_per_meter = 0x00000B12;
  header.info_header.y_pixels_per_meter = 0x00000B12;
  header.info_header.num_used_colors = 0;
  header.info_header.num_important_colors = 0;
  if (has_alpha) {
    // Write V5 header
    header.info_header.bitmask_r = 0x000000FF;
    header.info_header.bitmask_g = 0x0000FF00;
    header.info_header.bitmask_b = 0x00FF0000;
    header.info_header.bitmask_a = 0xFF000000;
    header.info_header.color_space_type = 0x73524742; // LCS_sRGB / 'sRGB'
    header.info_header.render_intent = 8; // LCS_GM_ABS_COLORIMETRIC
  }

  return header_size;
}

void Image::load(FILE* f) {
  char sig[2];
  freadx(f, sig, 2);

  Format format;
  bool is_extended_ppm = false;
  if (sig[0] == 'P' && sig[1] == '5') {
    format = Format::GRAYSCALE_PPM;
  } else if (sig[0] == 'P' && sig[1] == '6') {
    format = Format::COLOR_PPM;
  } else if (sig[0] == 'P' && sig[1] == '7') {
    format = Format::COLOR_PPM;
    is_extended_ppm = true;
  } else if (sig[0] == 'B' && sig[1] == 'M') {
    format = Format::WINDOWS_BITMAP;
  } else {
    throw unknown_format(string_printf(
        "can\'t load image; type signature is %02X%02X", sig[0], sig[1]));
  }

  if (format == Format::GRAYSCALE_PPM || format == Format::COLOR_PPM) {
    size_t new_width = 0;
    size_t new_height = 0;
    uint64_t new_max_value = 0;
    size_t new_depth = 0;

    if (is_extended_ppm) {
      if (fgetc(f) != '\n') {
        throw runtime_error("invalid extended PPM header");
      }
      for (;;) {
        string line = fgets(f);
        strip_trailing_whitespace(line);
        if (starts_with(line, "WIDTH ")) {
          new_width = stoull(line.substr(6));
        } else if (starts_with(line, "HEIGHT ")) {
          new_height = stoull(line.substr(7));
        } else if (starts_with(line, "DEPTH ")) {
          // We ignore this and use TUPLTYPE instead
        } else if (starts_with(line, "MAXVAL ")) {
          new_max_value = stoull(line.substr(7));
        } else if (starts_with(line, "TUPLTYPE ")) {
          string tuple_type = line.substr(9);
          if (tuple_type == "GRAYSCALE") {
            format = Format::GRAYSCALE_PPM;
            new_depth = 3;
          } else if (tuple_type == "GRAYSCALE_ALPHA") {
            format = Format::GRAYSCALE_PPM;
            new_depth = 4;
          } else if (tuple_type == "RGB") {
            format = Format::COLOR_PPM;
            new_depth = 3;
          } else if (tuple_type == "RGB_ALPHA") {
            format = Format::COLOR_PPM;
            new_depth = 4;
          } else {
            throw runtime_error("unsupported tuple type in extended PPM image");
          }
        } else if (line == "ENDHDR") {
          break;
        } else {
          throw runtime_error("unknown header command in extended PPM image");
        }
      }

    } else {
      new_depth = 3;
      if (fscanf(f, "%zu", &new_width) != 1) {
        throw runtime_error("cannot read width field in PPM header");
      }
      if (fscanf(f, "%zu", &new_height) != 1) {
        throw runtime_error("cannot read height field in PPM header");
      }
      if (fscanf(f, "%" PRIu64, &new_max_value) != 1) {
        throw runtime_error("cannot read max value field in PPM header");
      }

      // According to the docs, this is "usually" a newline but can be any
      // whitespace character, so we don't make that assumption here
      char header_end_char = fgetc(f);
      if ((header_end_char != ' ') &&
          (header_end_char != '\t') &&
          (header_end_char != '\n')) {
        throw runtime_error("whitespace character not present after PPM header");
      }
    }

    if (new_width == 0) {
      throw runtime_error("width field in PPM header is zero or missing");
    }
    if (new_height == 0) {
      throw runtime_error("height field in PPM header is zero or missing");
    }
    if (new_max_value == 0) {
      throw runtime_error("max value field in PPM header is zero or missing");
    }
    if (new_depth == 0) {
      throw runtime_error("depth or format field in PPM header is zero or missing");
    }
    if (new_depth < 3 || new_depth > 4) {
      throw runtime_error("depth or format field in PPM header has unsupported value");
    }
    bool new_has_alpha = (new_depth == 4);

    // The format docs say this is limited to 0xFFFF, but we'll support up to
    // 64-bit channels anyway
    uint8_t new_channel_width;
    if (new_max_value > 0xFFFFFFFF) {
      new_channel_width = 64;
    } else if (new_max_value > 0xFFFF) {
      new_channel_width = 32;
    } else if (new_max_value > 0xFF) {
      new_channel_width = 16;
    } else {
      new_channel_width = 8;
    }

    DataPtrs new_data;
    size_t channels_factor = (format == Format::COLOR_PPM ? 3 : 1) + (new_has_alpha ? 1 : 0);
    new_data.raw = malloc(new_width * new_height * channels_factor * (new_channel_width / 8));
    if (!new_data.raw) {
      throw bad_alloc();
    }
    try {
      freadx(f, new_data.raw,
          new_width * new_height * channels_factor * (new_channel_width / 8));
    } catch (const exception&) {
      free(new_data.raw);
      throw;
    }

    // If the read succeeded, we can commit the changes; nothing after here can
    // throw an exception.

    this->width = new_width;
    this->height = new_height;
    this->has_alpha = new_has_alpha;
    this->channel_width = new_channel_width;
    this->max_value = new_max_value;
    this->data.raw = new_data.raw;

    // Color PPM data is already in the necessary format for Image's internal
    // storage. Grayscale data is not - we have to expand it into color data. To
    // do so, we copy the gray channel to all color channels starting from the
    // end of the image (so we won't incorrectly overwrite unexpanded data).
    if (format == Format::GRAYSCALE_PPM) {
      size_t dest_stride = this->has_alpha ? 4 : 3;
      size_t src_stride = this->has_alpha ? 2 : 1;
      for (ssize_t y = this->height - 1; y >= 0; y--) {
        for (ssize_t x = this->width - 1; x >= 0; x--) {
          if (this->channel_width == 8) {
            uint8_t v = this->data.as8[y * this->width * src_stride + x];
            this->data.as8[(y * this->width + x) * dest_stride + 0] = v;
            this->data.as8[(y * this->width + x) * dest_stride + 1] = v;
            this->data.as8[(y * this->width + x) * dest_stride + 2] = v;
            if (this->has_alpha) {
              this->data.as8[(y * this->width + x) * dest_stride + 3] = this->data.as8[y * this->width * src_stride + x + 1];
            }
          } else if (this->channel_width == 16) {
            uint8_t v = this->data.as16[y * this->width * src_stride + x];
            this->data.as16[(y * this->width + x) * dest_stride + 0] = v;
            this->data.as16[(y * this->width + x) * dest_stride + 1] = v;
            this->data.as16[(y * this->width + x) * dest_stride + 2] = v;
            if (this->has_alpha) {
              this->data.as16[(y * this->width + x) * dest_stride + 3] = this->data.as16[y * this->width * src_stride + x + 1];
            }
          } else if (this->channel_width == 32) {
            uint8_t v = this->data.as32[y * this->width * src_stride + x];
            this->data.as32[(y * this->width + x) * dest_stride + 0] = v;
            this->data.as32[(y * this->width + x) * dest_stride + 1] = v;
            this->data.as32[(y * this->width + x) * dest_stride + 2] = v;
            if (this->has_alpha) {
              this->data.as32[(y * this->width + x) * dest_stride + 3] = this->data.as32[y * this->width * src_stride + x + 1];
            }
          } else if (this->channel_width == 64) {
            uint8_t v = this->data.as64[y * this->width * src_stride + x];
            this->data.as64[(y * this->width + x) * dest_stride + 0] = v;
            this->data.as64[(y * this->width + x) * dest_stride + 1] = v;
            this->data.as64[(y * this->width + x) * dest_stride + 2] = v;
            if (this->has_alpha) {
              this->data.as64[(y * this->width + x) * dest_stride + 3] = this->data.as64[y * this->width * src_stride + x + 1];
            }
          }
        }
      }
    }

  } else if (format == Format::WINDOWS_BITMAP) {
    WindowsBitmapHeader header = {};

    // Read BMP file header
    memcpy(&header.file_header.magic, sig, 2);
    freadx(f, reinterpret_cast<uint8_t*>(&header.file_header) + 2, sizeof(header.file_header) - 2);
    if (header.file_header.magic != 0x4D42) {
      throw runtime_error(string_printf("bad signature in bitmap file (%04hX)",
          header.file_header.magic.load()));
    }

    // Read variable-sized BMP info header
    freadx(f, &header.info_header.header_size, 4);
    if (header.info_header.header_size > sizeof(header.info_header)) {
      throw runtime_error(string_printf("unsupported bitmap header: size is %u, maximum supported size is %zu",
          header.info_header.header_size.load(), sizeof(header.info_header)));
    }

    freadx(f, reinterpret_cast<uint8_t*>(&header.info_header) + 4, header.info_header.header_size - 4);
    if ((header.info_header.bit_depth != 24) && (header.info_header.bit_depth != 32)) {
      throw runtime_error(string_printf(
          "can only load 24-bit or 32-bit bitmaps (this is a %hu-bit bitmap)",
          header.info_header.bit_depth.load()));
    }
    if (header.info_header.num_planes != 1) {
      throw runtime_error("can only load 1-plane bitmaps");
    }

    bool reverse_row_order = header.info_header.height < 0;
    fseek(f, header.file_header.data_offset, SEEK_SET);
    bool has_alpha;
    int32_t w = header.info_header.width;
    int32_t h = header.info_header.height * (reverse_row_order ? -1 : 1);
    unique_ptr<void, void (*)(void*)> new_data_unique(nullptr, free);

    if (header.info_header.compression == 0) { // BI_RGB
      if ((header.info_header.bit_depth != 24) && (header.info_header.bit_depth != 32)) {
        throw runtime_error("bitmap uses BI_RGB but bit depth is not 24 or 32");
      }

      has_alpha = false;
      size_t pixel_bytes = header.info_header.bit_depth / 8;
      size_t row_padding_bytes = (4 - ((w * pixel_bytes) % 4)) % 4;

      new_data_unique = malloc_unique(w * h * 3);
      uint8_t* new_data = reinterpret_cast<uint8_t*>(new_data_unique.get());

      auto row_data_unique = malloc_unique(w * pixel_bytes);
      uint8_t* row_data = reinterpret_cast<uint8_t*>(row_data_unique.get());
      for (int32_t y = h - 1; y >= 0; y--) {
        freadx(f, row_data, w * pixel_bytes);
        ssize_t target_y = reverse_row_order ? (h - y - 1) : y;
        ssize_t target_y_offset = target_y * w * 3;
        for (int32_t x = 0; x < w; x++) {
          size_t x_offset = x * 3;
          size_t src_x_offset = x * pixel_bytes;
          new_data[target_y_offset + x_offset + 2] = row_data[src_x_offset + 0];
          new_data[target_y_offset + x_offset + 1] = row_data[src_x_offset + 1];
          new_data[target_y_offset + x_offset + 0] = row_data[src_x_offset + 2];
        }
        if (row_padding_bytes) {
          fseek(f, row_padding_bytes, SEEK_CUR);
        }
      }

    } else if (header.info_header.compression == 3) { // BI_BITFIELDS
      if (header.info_header.bit_depth != 32) {
        throw runtime_error("bitmap uses BI_BITFIELDS but bit depth is not 32");
      }

      has_alpha = true;

      // we only support bitmaps where channels are entire bytes
      // note the offsets are reversed because little-endian
      size_t r_offset, g_offset, b_offset, a_offset;
      unordered_map<uint32_t, size_t> offset_for_bitmask({{0xFF000000, 3}, {0x00FF0000, 2}, {0x0000FF00, 1}, {0x000000FF, 0}});
      try {
        r_offset = offset_for_bitmask.at(header.info_header.bitmask_r);
        g_offset = offset_for_bitmask.at(header.info_header.bitmask_g);
        b_offset = offset_for_bitmask.at(header.info_header.bitmask_b);
        a_offset = offset_for_bitmask.at(header.info_header.bitmask_a);
      } catch (const out_of_range&) {
        throw runtime_error("channel bit field is not 1-byte mask");
      }

      new_data_unique = malloc_unique(w * h * 4);
      uint8_t* new_data = reinterpret_cast<uint8_t*>(new_data_unique.get());

      auto row_data_unique = malloc_unique(w * 4);
      uint8_t* row_data = reinterpret_cast<uint8_t*>(row_data_unique.get());
      for (int32_t y = h - 1; y >= 0; y--) {
        freadx(f, row_data, w * 4);
        ssize_t target_y = reverse_row_order ? (h - y - 1) : y;
        ssize_t target_y_offset = target_y * w * 4;
        for (int32_t x = 0; x < w; x++) {
          size_t x_offset = x * 4;
          new_data[target_y_offset + x_offset + 0] = row_data[x_offset + r_offset];
          new_data[target_y_offset + x_offset + 1] = row_data[x_offset + g_offset];
          new_data[target_y_offset + x_offset + 2] = row_data[x_offset + b_offset];
          new_data[target_y_offset + x_offset + 3] = row_data[x_offset + a_offset];
        }
      }

    } else {
      throw runtime_error("can only load uncompressed or bitfield bitmaps");
    }

    // load was successful; commit the changes
    this->width = w;
    this->height = h;
    this->has_alpha = has_alpha;
    this->channel_width = 8;
    this->max_value = 0xFF;
    this->data.raw = new_data_unique.release();
  }
}

static constexpr uint64_t mask_for_width(uint8_t channel_width) {
  return 0xFFFFFFFFFFFFFFFFULL >> (64 - channel_width);
}

Image::Image()
    : width(0),
      height(0),
      has_alpha(false),
      channel_width(8),
      max_value(mask_for_width(this->channel_width)) {
  this->data.raw = nullptr;
}

Image::Image(size_t x, size_t y, bool has_alpha, uint8_t channel_width)
    : width(x),
      height(y),
      has_alpha(has_alpha),
      channel_width(channel_width),
      max_value(mask_for_width(this->channel_width)) {
  size_t num_bytes = this->get_data_size();
  this->data.raw = malloc(num_bytes);
  memset(this->data.raw, 0, num_bytes * sizeof(uint8_t));
}

Image::Image(const Image& im)
    : width(im.width),
      height(im.height),
      has_alpha(im.has_alpha),
      channel_width(im.channel_width),
      max_value(im.max_value) {
  size_t num_bytes = this->get_data_size();
  this->data.raw = malloc(num_bytes);
  memcpy(this->data.raw, im.data.raw, num_bytes * sizeof(uint8_t));
}

const Image& Image::operator=(const Image& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->channel_width = im.channel_width;
  this->max_value = im.max_value;
  size_t num_bytes = this->get_data_size();
  if (this->data.raw) {
    free(this->data.raw);
  }
  this->data.raw = malloc(num_bytes);
  memcpy(this->data.raw, im.data.raw, num_bytes * sizeof(uint8_t));
  return *this;
}

Image::Image(Image&& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->channel_width = im.channel_width;
  this->max_value = im.max_value;
  this->data = im.data;

  im.width = 0;
  im.height = 0;
  im.has_alpha = false;
  im.channel_width = 8;
  im.max_value = 0xFF;
  im.data.raw = nullptr;
}

Image& Image::operator=(Image&& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->channel_width = im.channel_width;
  this->max_value = im.max_value;
  if (this->data.raw) {
    free(this->data.raw);
  }
  this->data.raw = im.data.raw;

  im.width = 0;
  im.height = 0;
  im.has_alpha = false;
  im.channel_width = 8;
  im.max_value = 0xFF;
  im.data.raw = nullptr;
  return *this;
}

Image::Image(FILE* f) {
  this->load(f);
}

Image::Image(const char* filename) {
  if (filename) {
    auto f = fopen_unique(filename, "rb");
    this->load(f.get());
  } else {
    this->load(stdin);
  }
}

Image::Image(const string& filename) : Image(filename.c_str()) {}

Image::Image(FILE* f, ssize_t width, ssize_t height, bool has_alpha,
    uint8_t channel_width, uint64_t max_value)
    : width(width),
      height(height),
      has_alpha(has_alpha),
      channel_width(channel_width),
      max_value(max_value ? max_value : mask_for_width(this->channel_width)) {
  size_t num_bytes = this->get_data_size();
  this->data.raw = malloc(num_bytes);
  freadx(f, this->data.raw, num_bytes);
}

Image::Image(const char* filename, ssize_t width, ssize_t height,
    bool has_alpha, uint8_t channel_width, uint64_t max_value)
    : width(width),
      height(height),
      has_alpha(has_alpha),
      channel_width(channel_width),
      max_value(max_value ? max_value : mask_for_width(this->channel_width)) {
  auto f = fopen_unique(filename, "rb");
  size_t num_bytes = this->get_data_size();
  this->data.raw = malloc(num_bytes);
  freadx(f.get(), this->data.raw, num_bytes);
}

Image::Image(const std::string& filename, ssize_t width, ssize_t height,
    bool has_alpha, uint8_t channel_width, uint64_t max_value) : Image(filename.c_str(), width, height, has_alpha, channel_width, max_value) {}

Image::~Image() {
  free(this->data.raw);
}

bool Image::operator==(const Image& other) const {
  if ((this->width != other.width) || (this->height != other.height) ||
      (this->has_alpha != other.has_alpha) ||
      (this->channel_width != other.channel_width) ||
      (this->max_value != other.max_value)) {
    return false;
  }
  return !memcmp(this->data.raw, other.data.raw, this->get_data_size());
}

bool Image::operator!=(const Image& other) const {
  return !this->operator==(other);
}

const char* Image::mime_type_for_format(Format format) {
  switch (format) {
    case Format::GRAYSCALE_PPM:
    case Format::COLOR_PPM:
      return "image/x-portable-pixmap";
    case Format::WINDOWS_BITMAP:
      return "image/bmp";
    case Format::PNG:
      return "image/png";
    default:
      return "text/plain";
  }
}

const char* Image::file_extension_for_format(Format format) {
  switch (format) {
    case Format::GRAYSCALE_PPM:
    case Format::COLOR_PPM:
      return "ppm";
    case Format::WINDOWS_BITMAP:
      return "bmp";
    case Format::PNG:
      return "png";
    default:
      return "raw";
  }
}

template <typename Writer>
static void write_png_chunk(const char (&type)[5],
    const void* data, be_uint32_t size,
    Writer&& writer) {

  writer(&size, 4);
  writer(type, 4);
  if (size > 0) {
    writer(data, size);
  }

  // The CRC includes the chunk type, but not the length
  // The CRC of the (empty) IEND chunk is big-endian 0xAE426082
  be_uint32_t crc = crc32(0u, reinterpret_cast<const Bytef*>(type), 4);
  if (size > 0) {
    crc = crc32(crc, static_cast<const Bytef*>(data), size);
  }
  writer(&crc, 4);
}

template <typename Writer>
void Image::save_helper(Format format, Writer&& writer) const {
  switch (format) {
    case Format::GRAYSCALE_PPM:
      throw runtime_error("can\'t save grayscale ppm files");

    case Format::COLOR_PPM: {
      char header[256];
      if (this->has_alpha) {
        snprintf(header, sizeof(header), "P7\nWIDTH %zu\nHEIGHT %zu\nDEPTH 4\nMAXVAL %" PRIu64 "\nTUPLTYPE RGB_ALPHA\nENDHDR\n",
            this->width, this->height, this->max_value);
      } else {
        snprintf(header, sizeof(header), "P6 %zu %zu %" PRIu64 "\n", this->width, this->height, this->max_value);
      }
      writer(header, strlen(header));
      writer(this->data.raw, this->get_data_size());
      break;
    }

    case Format::WINDOWS_BITMAP: {
      if (this->channel_width != 8) {
        throw runtime_error("can\'t save bmp with more than 8-bit channels");
      }

      size_t pixel_bytes = 3 + this->has_alpha;
      size_t row_padding_bytes = (4 - ((this->width * pixel_bytes) % 4)) % 4;
      uint8_t row_padding_data[4] = {0, 0, 0, 0};

      WindowsBitmapHeader header;
      size_t header_size = init_bmp_header(header, this->width, this->height, this->has_alpha, pixel_bytes, row_padding_bytes);

      writer(&header, header_size);

      if (this->has_alpha) {
        // there's no padding and the bitmasks already specify how to read each
        // pixel; just write each row
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          writer(&this->data.as8[y * this->width * 4], this->width * 4);
        }

      } else {
        auto row_data_unique = malloc_unique(this->width * 3);
        uint8_t* row_data = reinterpret_cast<uint8_t*>(row_data_unique.get());
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = 0; x < this->width * 3; x += 3) {
            row_data[x] = this->data.as8[y * this->width * 3 + x + 2];
            row_data[x + 1] = this->data.as8[y * this->width * 3 + x + 1];
            row_data[x + 2] = this->data.as8[y * this->width * 3 + x];
          }
          writer(row_data, this->width * 3);
          if (row_padding_bytes) {
            writer(row_padding_data, row_padding_bytes);
          }
        }
      }
      break;
    }

    case Format::PNG: {
      if (this->channel_width != 8) {
        throw runtime_error("can\'t save png with more than 8-bit channels");
      }

      constexpr uint8_t SIG[8] = {137, 80, 78, 71, 13, 10, 26, 10};

      writer(SIG, sizeof(SIG));

      // Write IDHR chunk
      const struct {
        be_uint32_t width;
        be_uint32_t height;
        uint8_t bit_depth;
        uint8_t color_type;
        uint8_t compression;
        uint8_t filter;
        uint8_t interlace;
      } IHDR{
          this->width,
          this->height,
          8,
          uint8_t(this->has_alpha ? 6 : 2),
          0, // default compression
          0, // default filter
          0 // non-interlaced
      };
      write_png_chunk("IHDR", &IHDR, 13, writer);

      // Write gAMA chunk
      const be_uint32_t gAMA = 45455; // 1/2.2
      write_png_chunk("gAMA", &gAMA, sizeof(gAMA), writer);

      // Write IDAT chunk
      size_t pixel_size = 3 + this->has_alpha;
      size_t image_size = this->height * (1 + this->width * pixel_size);
      auto image_data = malloc_unique(image_size);

      for (unsigned int y = 0; y < static_cast<size_t>(this->height); ++y) {
        const uint8_t* s = this->data.as8 + y * this->width * pixel_size;
        uint8_t* d = static_cast<uint8_t*>(image_data.get()) + y * (1 + this->width * pixel_size);
        *d = 0; // no filter
        memcpy(d + 1, s, this->width * pixel_size);
      }

      uLongf idat_size = compressBound(image_size);
      auto idat_data = malloc_unique(idat_size);
      if (int result = compress2(static_cast<Bytef*>(idat_data.get()), &idat_size, static_cast<const Bytef*>(image_data.get()), image_size, 9)) {
        throw runtime_error(string_printf("zlib error compressing png data: %d", result));
      }

      write_png_chunk("IDAT", idat_data.get(), idat_size, writer);

      // Write IEND chunk
      write_png_chunk("IEND", nullptr, 0, writer);
      break;
    }

    default:
      throw runtime_error("unknown file format in Image::save()");
  }
}

// save the image to an already-open file
void Image::save(FILE* f, Format format) const {
  this->save_helper(format, [f](const void* data, size_t size) {
    fwritex(f, data, size);
  });
}

// save the image to a string in memory
string Image::save(Format format) const {
  string result;
  this->save_helper(format, [&result](const void* data, size_t size) {
    result.append(reinterpret_cast<const char*>(data), size);
  });
  return result;
}

// saves the Image. if nullptr is given for filename, writes to stdout
void Image::save(const char* filename, Format format) const {
  if (filename) {
    auto f = fopen_unique(filename, "wb");
    this->save(f.get(), format);
  } else {
    this->save(stdout, format);
  }
}

void Image::save(const string& filename, Format format) const {
  auto f = fopen_unique(filename, "wb");
  this->save(f.get(), format);
}

string Image::png_data_url() const {
  string png_data = this->save(Format::PNG);
  string base64_data = base64_encode(png_data);
  return "data:image/png;base64," + base64_data;
}

void Image::clear(uint64_t r, uint64_t g, uint64_t b, uint64_t a) {
  for (ssize_t y = 0; y < this->height; y++) {
    for (ssize_t x = 0; x < this->width; x++) {
      this->write_pixel(x, y, r, g, b, a);
    }
  }
}
void Image::clear(uint32_t c) {
  auto ec = expand_color(c);
  this->clear(ec.r, ec.g, ec.b, ec.a);
}

// read the specified pixel's rgb values
void Image::read_pixel(ssize_t x, ssize_t y, uint64_t* r, uint64_t* g,
    uint64_t* b, uint64_t* a) const {

  // check coordinates
  if (x < 0 || y < 0 || x >= this->width || y >= this->height) {
    throw runtime_error("out of bounds");
  }

  size_t index = (y * this->width + x) * (this->has_alpha ? 4 : 3);
  if (this->channel_width == 8) {
    if (r) {
      *r = this->data.as8[index];
    }
    if (g) {
      *g = this->data.as8[index + 1];
    }
    if (b) {
      *b = this->data.as8[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data.as8[index + 3] : this->max_value;
    }
  } else if (this->channel_width == 16) {
    if (r) {
      *r = this->data.as16[index];
    }
    if (g) {
      *g = this->data.as16[index + 1];
    }
    if (b) {
      *b = this->data.as16[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data.as16[index + 3] : this->max_value;
    }
  } else if (this->channel_width == 32) {
    if (r) {
      *r = this->data.as32[index];
    }
    if (g) {
      *g = this->data.as32[index + 1];
    }
    if (b) {
      *b = this->data.as32[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data.as32[index + 3] : this->max_value;
    }
  } else if (this->channel_width == 64) {
    if (r) {
      *r = this->data.as64[index];
    }
    if (g) {
      *g = this->data.as64[index + 1];
    }
    if (b) {
      *b = this->data.as64[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data.as64[index + 3] : this->max_value;
    }
  } else {
    throw logic_error("image channel width is not 8, 16, 32, or 64");
  }
}
uint32_t Image::read_pixel(ssize_t x, ssize_t y) const {
  // TODO: This probably doesn't optimize well; we should directly read from the
  // color data here
  uint64_t r, g, b, a;
  this->read_pixel(x, y, &r, &g, &b, &a);
  return compress_color(r, g, b, a);
}

// write the specified pixel's rgb values
void Image::write_pixel(ssize_t x, ssize_t y, uint64_t r, uint64_t g,
    uint64_t b, uint64_t a) {

  // check coordinates
  if (x < 0 || y < 0 || x >= this->width || y >= this->height) {
    throw runtime_error("out of bounds");
  }

  size_t index = (y * this->width + x) * (this->has_alpha ? 4 : 3);
  if (this->channel_width == 8) {
    this->data.as8[index] = r;
    this->data.as8[index + 1] = g;
    this->data.as8[index + 2] = b;
    if (this->has_alpha) {
      this->data.as8[index + 3] = a;
    }
  } else if (this->channel_width == 16) {
    this->data.as16[index] = r;
    this->data.as16[index + 1] = g;
    this->data.as16[index + 2] = b;
    if (this->has_alpha) {
      this->data.as16[index + 3] = a;
    }
  } else if (this->channel_width == 32) {
    this->data.as32[index] = r;
    this->data.as32[index + 1] = g;
    this->data.as32[index + 2] = b;
    if (this->has_alpha) {
      this->data.as32[index + 3] = a;
    }
  } else if (this->channel_width == 64) {
    this->data.as64[index] = r;
    this->data.as64[index + 1] = g;
    this->data.as64[index + 2] = b;
    if (this->has_alpha) {
      this->data.as64[index + 3] = a;
    }
  } else {
    throw logic_error("image channel width is not 8, 16, 32, or 64");
  }
}

void Image::write_pixel(ssize_t x, ssize_t y, uint32_t color) {
  auto ec = expand_color(color);
  this->write_pixel(x, y, ec.r, ec.g, ec.b, ec.a);
}

void Image::reverse_horizontal() {
  for (ssize_t y = 0; y < this->height; y++) {
    for (ssize_t x = 0; x < this->width / 2; x++) {
      uint64_t r1, g1, b1, a1, r2, g2, b2, a2;
      this->read_pixel(x, y, &r1, &g1, &b1, &a1);
      this->read_pixel(this->width - x - 1, y, &r2, &g2, &b2, &a2);
      this->write_pixel(this->width - x - 1, y, r1, g1, b1, a1);
      this->write_pixel(x, y, r2, g2, b2, a2);
    }
  }
}

void Image::reverse_vertical() {
  for (ssize_t y = 0; y < this->height / 2; y++) {
    for (ssize_t x = 0; x < this->width; x++) {
      uint64_t r1, g1, b1, a1, r2, g2, b2, a2;
      this->read_pixel(x, y, &r1, &g1, &b1, &a1);
      this->read_pixel(x, this->height - y - 1, &r2, &g2, &b2, &a2);
      this->write_pixel(x, this->height - y - 1, r1, g1, b1, a1);
      this->write_pixel(x, y, r2, g2, b2, a2);
    }
  }
}

void Image::set_channel_width(uint8_t new_width) {
  if (new_width != 8 && new_width != 16 && new_width != 32 && new_width != 64) {
    throw runtime_error("channel width must be 8, 16, 32, or 64");
  }
  if (this->channel_width == new_width) {
    return;
  }

  DataPtrs new_data;
  new_data.raw = malloc(this->width * this->height * (3 + this->has_alpha) * (new_width / 8));

  size_t value_count = this->width * this->height * (this->has_alpha ? 4 : 3);
  for (size_t z = 0; z < value_count; z++) {
    uint64_t v;
    // If the new channel width is larger than the current width, expand the
    // channels by copying the now-high bits to the lower bits. If the new
    // channel width is smaller, preserve only the high bits of the original
    // values.
    if (this->channel_width == 8) {
      v = this->data.as8[z];
      if (new_width == 16) {
        new_data.as16[z] = (v << 8) | v;
      } else if (new_width == 32) {
        new_data.as32[z] = (v << 24) | (v << 16) | (v << 8) | v;
      } else if (new_width == 64) {
        new_data.as64[z] = (v << 56) | (v << 48) | (v << 40) | (v << 32) | (v << 24) | (v << 16) | (v << 8) | v;
      }

    } else if (this->channel_width == 16) {
      v = this->data.as16[z];
      if (new_width == 8) {
        new_data.as8[z] = v >> 8;
      } else if (new_width == 32) {
        new_data.as32[z] = (v << 16) | v;
      } else if (new_width == 64) {
        new_data.as64[z] = (v << 48) | (v << 32) | (v << 16) | v;
      }

    } else if (this->channel_width == 32) {
      v = this->data.as32[z];
      if (new_width == 8) {
        new_data.as8[z] = v >> 24;
      } else if (new_width == 16) {
        new_data.as16[z] = v >> 16;
      } else if (new_width == 64) {
        new_data.as64[z] = (v << 32) | v;
      }

    } else if (this->channel_width == 64) {
      v = this->data.as64[z];
      if (new_width == 8) {
        new_data.as8[z] = v >> 56;
      } else if (new_width == 16) {
        new_data.as16[z] = v >> 48;
      } else if (new_width == 32) {
        new_data.as32[z] = v >> 32;
      }
    }
  }

  free(this->data.raw);
  this->data.raw = new_data.raw;
  this->channel_width = new_width;
  this->max_value = mask_for_width(this->channel_width);
}

void Image::set_has_alpha(bool new_has_alpha) {
  if (this->has_alpha == new_has_alpha) {
    return;
  }
  this->has_alpha = new_has_alpha;

  DataPtrs new_data;
  new_data.raw = malloc(this->get_data_size());

  for (ssize_t z = 0; z < this->width * this->height; z++) {
    size_t src_index = z * (this->has_alpha ? 3 : 4);
    size_t dst_index = z * (this->has_alpha ? 4 : 3);

    if (this->channel_width == 8) {
      new_data.as8[dst_index + 0] = this->data.as8[src_index + 0];
      new_data.as8[dst_index + 1] = this->data.as8[src_index + 1];
      new_data.as8[dst_index + 2] = this->data.as8[src_index + 2];
      if (this->has_alpha) {
        new_data.as8[dst_index + 3] = this->max_value;
      }
    } else if (this->channel_width == 16) {
      new_data.as16[dst_index + 0] = this->data.as16[src_index + 0];
      new_data.as16[dst_index + 1] = this->data.as16[src_index + 1];
      new_data.as16[dst_index + 2] = this->data.as16[src_index + 2];
      if (this->has_alpha) {
        new_data.as16[dst_index + 3] = this->max_value;
      }
    } else if (this->channel_width == 32) {
      new_data.as32[dst_index + 0] = this->data.as32[src_index + 0];
      new_data.as32[dst_index + 1] = this->data.as32[src_index + 1];
      new_data.as32[dst_index + 2] = this->data.as32[src_index + 2];
      if (this->has_alpha) {
        new_data.as32[dst_index + 3] = this->max_value;
      }
    } else if (this->channel_width == 64) {
      new_data.as64[dst_index + 0] = this->data.as64[src_index + 0];
      new_data.as64[dst_index + 1] = this->data.as64[src_index + 1];
      new_data.as64[dst_index + 2] = this->data.as64[src_index + 2];
      if (this->has_alpha) {
        new_data.as64[dst_index + 3] = this->max_value;
      }
    }
  }

  free(this->data.raw);
  this->data.raw = new_data.raw;
}

void Image::set_alpha_from_mask_color(uint64_t r, uint64_t g, uint64_t b) {
  ssize_t w = this->get_width();
  ssize_t h = this->get_height();
  for (ssize_t y = 0; y < h; y++) {
    for (ssize_t x = 0; x < w; x++) {
      uint64_t sr, sg, sb;
      this->read_pixel(x, y, &sr, &sg, &sb);
      uint64_t a = (sr == r && sg == g && sb == b) ? 0 : this->max_value;
      this->write_pixel(x, y, sr, sg, sb, a);
    }
  }
}

void Image::set_alpha_from_mask_color(uint32_t c) {
  auto ec = expand_color(c);
  this->set_alpha_from_mask_color(ec.r, ec.g, ec.b);
}

// use the Bresenham algorithm to draw a line between the specified points
void Image::draw_line(ssize_t x0, ssize_t y0, ssize_t x1, ssize_t y1,
    uint64_t r, uint64_t g, uint64_t b, uint64_t a) {

  // if both endpoints are outside the image, don't bother
  if ((x0 < 0 || x0 >= width || y0 < 0 || y0 >= height) &&
      (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height)) {
    return;
  }

  // line is too steep? then we step along y rather than x
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    ssize_t t = x0;
    x0 = y0;
    y0 = t;
    t = x1;
    x1 = y1;
    y1 = t;
  }

  // line is backward? then switch the points
  if (x0 > x1) {
    ssize_t t = x1;
    x1 = x0;
    x0 = t;
    t = y1;
    y1 = y0;
    y0 = t;
  }

  // initialize variables for stepping along the line
  ssize_t dx = x1 - x0;
  ssize_t dy = abs(y1 - y0);
  double error = 0;
  double derror = (double)dy / (double)dx;
  ssize_t ystep = (y0 < y1) ? 1 : -1;
  ssize_t y = y0;

  // now walk along the line
  for (ssize_t x = x0; x <= x1; x++) {
    if (steep) {
      try {
        this->write_pixel(y, x, r, g, b, a);
      } catch (const runtime_error& e) {
        return;
      }
    } else {
      try {
        this->write_pixel(x, y, r, g, b, a);
      } catch (const runtime_error& e) {
        return;
      }
    }
    error += derror;

    // have we passed the center of this row? then move to the next row
    if (error >= 0.5) {
      y += ystep;
      error -= 1.0;
    }
  }
}
void Image::draw_line(ssize_t x0, ssize_t y0, ssize_t x1, ssize_t y1,
    uint32_t c) {
  auto ec = expand_color(c);
  this->draw_line(x0, y0, x1, y1, ec.r, ec.g, ec.b, ec.a);
}

void Image::draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y,
    ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b, uint64_t a) {

  for (ssize_t x = x1; x <= x2; x++) {
    if (dash_length && ((x / dash_length) & 1)) {
      continue;
    }
    try {
      this->write_pixel(x, y, r, g, b, a);
    } catch (const runtime_error& e) {
      break;
    }
  }
}
void Image::draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y,
    ssize_t dash_length, uint32_t c) {
  auto ec = expand_color(c);
  this->draw_horizontal_line(x1, x2, y, dash_length, ec.r, ec.g, ec.b, ec.a);
}

void Image::draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2,
    ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b, uint64_t a) {

  for (ssize_t y = y1; y <= y2; y++) {
    if (dash_length && ((y / dash_length) & 1)) {
      continue;
    }
    try {
      this->write_pixel(x, y, r, g, b, a);
    } catch (const runtime_error& e) {
      break;
    }
  }
}
void Image::draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2,
    ssize_t dash_length, uint32_t c) {
  auto ec = expand_color(c);
  this->draw_vertical_line(x, y1, y2, dash_length, ec.r, ec.g, ec.b, ec.a);
}

void Image::draw_text_v(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
    uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
    uint64_t bb, uint64_t ba, const char* fmt, va_list va) {

  string buffer = string_vprintf(fmt, va);

  ssize_t max_x_pos = 0;
  ssize_t x_pos = x, y_pos = y;
  for (size_t z = 0; z < buffer.size(); z++) {
    uint8_t ch = buffer[z];
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      if (ba) {
        this->fill_rect(x_pos - 1, y_pos - 1, 1, 9, br, bg, bb, ba);
      }
      y_pos += 8;
      x_pos = x;
      if (x_pos > max_x_pos) {
        max_x_pos = x_pos;
      }
      continue;
    }

    if (ch < 0x20 || ch > 0x7F) {
      ch = 0x7F;
    }
    ch -= 0x20;

    if (ba) {
      this->fill_rect(x_pos - 1, y_pos - 1, 6, 9, br, bg, bb, ba);
    }
    for (ssize_t yy = 0; yy < 7; yy++) {
      for (ssize_t xx = 0; xx < 5; xx++) {
        if (!font[ch][yy * 5 + xx]) {
          continue;
        }
        try {
          this->write_pixel(x_pos + xx, y_pos + yy, r, g, b, a);
        } catch (const runtime_error& e) {
        }
      }
    }

    x_pos += 6;
  }

  this->fill_rect(x_pos - 1, y_pos - 1, 1, 9, br, bg, bb, ba);

  if (width) {
    *width = (x_pos > max_x_pos ? x_pos : max_x_pos) - x;
  }
  if (height) {
    *height = y_pos + 7 - y;
  }
}

void Image::draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
    uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
    uint64_t bb, uint64_t ba, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  try {
    this->draw_text_v(x, y, width, height, r, g, b, a, br, bg, bb, ba, fmt, va);
  } catch (...) {
    va_end(va);
    throw;
  }
  va_end(va);
}

void Image::draw_text(ssize_t x, ssize_t y,
    uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
    uint64_t bb, uint64_t ba, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  try {
    this->draw_text_v(x, y, nullptr, nullptr, r, g, b, a, br, bg, bb, ba, fmt, va);
  } catch (...) {
    va_end(va);
    throw;
  }
  va_end(va);
}

void Image::draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
    uint32_t color, uint32_t background, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  try {
    this->draw_text_v(x, y, width, height,
        (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF,
        (background >> 24) & 0xFF, (background >> 16) & 0xFF, (background >> 8) & 0xFF, background & 0xFF,
        fmt, va);
  } catch (...) {
    va_end(va);
    throw;
  }
  va_end(va);
}

void Image::draw_text(ssize_t x, ssize_t y, uint32_t color, uint32_t background,
    const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  try {
    this->draw_text_v(x, y, nullptr, nullptr,
        (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF,
        (background >> 24) & 0xFF, (background >> 16) & 0xFF, (background >> 8) & 0xFF, background & 0xFF,
        fmt, va);
  } catch (...) {
    va_end(va);
    throw;
  }
  va_end(va);
}

void Image::draw_text(ssize_t x, ssize_t y, uint32_t color, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  try {
    this->draw_text_v(x, y, nullptr, nullptr,
        (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF,
        0, 0, 0, 0, fmt, va);
  } catch (...) {
    va_end(va);
    throw;
  }
  va_end(va);
}

void Image::fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint64_t r,
    uint64_t g, uint64_t b, uint64_t a) {

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > static_cast<ssize_t>(this->get_width())) {
    w = this->get_width() - x;
  }
  if (y + h > static_cast<ssize_t>(this->get_height())) {
    h = this->get_height() - y;
  }

  if (a == 0xFF) {
    for (ssize_t yy = 0; yy < h; yy++) {
      for (ssize_t xx = 0; xx < w; xx++) {
        try {
          this->write_pixel(x + xx, y + yy, r, g, b, a);
        } catch (const runtime_error& e) {
        }
      }
    }
  } else {
    for (ssize_t yy = 0; yy < h; yy++) {
      for (ssize_t xx = 0; xx < w; xx++) {
        try {
          uint64_t _r = 0, _g = 0, _b = 0, _a = 0;
          this->read_pixel(x + xx, y + yy, &_r, &_g, &_b, &_a);
          _r = (a * (uint32_t)r + (0xFF - a) * (uint32_t)_r) / 0xFF;
          _g = (a * (uint32_t)g + (0xFF - a) * (uint32_t)_g) / 0xFF;
          _b = (a * (uint32_t)b + (0xFF - a) * (uint32_t)_b) / 0xFF;
          _a = (a * (uint32_t)a + (0xFF - a) * (uint32_t)_a) / 0xFF;
          this->write_pixel(x + xx, y + yy, _r, _g, _b, _a);
        } catch (const runtime_error& e) {
        }
      }
    }
  }
}
void Image::fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint32_t c) {
  auto ec = expand_color(c);
  this->fill_rect(x, y, w, h, ec.r, ec.g, ec.b, ec.a);
}

void Image::invert() {
  ssize_t w = this->get_width();
  ssize_t h = this->get_height();
  for (ssize_t y = 0; y < h; y++) {
    for (ssize_t x = 0; x < w; x++) {
      uint64_t r, g, b, a;
      this->read_pixel(x, y, &r, &g, &b, &a);
      this->write_pixel(x, y, this->max_value - r, this->max_value - g, this->max_value - b, this->max_value - a);
    }
  }
}

static inline void clamp_blit_dimensions(
    const Image& dest, const Image& source, ssize_t* x, ssize_t* y, ssize_t* w,
    ssize_t* h, ssize_t* sx, ssize_t* sy) {
  // If the source coordinates are negative, trim off the left/top
  if (*sx < 0) {
    *x -= *sx;
    *w += *sx;
    *sx = 0;
  }
  if (*sy < 0) {
    *y -= *sy;
    *h += *sy;
    *sy = 0;
  }
  // If the dest coordinates are negative, trim off the left/top
  if (*x < 0) {
    *sx -= *x;
    *w += *x;
    *x = 0;
  }
  if (*y < 0) {
    *sy -= *y;
    *h += *y;
    *y = 0;
  }
  // If the area extends beyond the source, trim off the right/bottom
  if (*sx + *w > static_cast<ssize_t>(source.get_width())) {
    *w = source.get_width() - *sx;
  }
  if (*sy + *h > static_cast<ssize_t>(source.get_height())) {
    *h = source.get_height() - *sy;
  }
  // If the area extends beyond the dest, trim off the right/bottom
  if (*x + *w > static_cast<ssize_t>(dest.get_width())) {
    *w = dest.get_width() - *x;
  }
  if (*y + *h > static_cast<ssize_t>(dest.get_height())) {
    *h = dest.get_height() - *y;
  }

  // If either width or height are negative, then the entire area is out of
  // bounds for either the source or dest - make the area empty
  if (*w < 0 || *h < 0) {
    *w = 0;
    *h = 0;
  }
}

void Image::blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy) {

  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (ssize_t yy = 0; yy < h; yy++) {
    for (ssize_t xx = 0; xx < w; xx++) {
      uint64_t r, g, b, a;
      source.read_pixel(sx + xx, sy + yy, &r, &g, &b, &a);
      if (a == 0) {
        continue;
      }
      if (a != 0xFF) {
        uint64_t sr, sg, sb, sa;
        this->read_pixel(x + xx, y + yy, &sr, &sg, &sb, &sa);
        r = (a * (uint32_t)r + (0xFF - a) * (uint32_t)sr) / 0xFF;
        g = (a * (uint32_t)g + (0xFF - a) * (uint32_t)sg) / 0xFF;
        b = (a * (uint32_t)b + (0xFF - a) * (uint32_t)sb) / 0xFF;
        a = (a * (uint32_t)a + (0xFF - a) * (uint32_t)sa) / 0xFF;
      }
      this->write_pixel(x + xx, y + yy, r, g, b, a);
    }
  }
}

void Image::mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b) {

  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint64_t _r, _g, _b, _a;
      source.read_pixel(sx + xx, sy + yy, &_r, &_g, &_b, &_a);
      if (r != _r || g != _g || b != _b) {
        this->write_pixel(x + xx, y + yy, _r, _g, _b, _a);
      }
    }
  }
}
void Image::mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, uint32_t transparent_c) {
  auto ec = expand_color(transparent_c);
  this->mask_blit(source, x, y, w, h, sx, sy, ec.r, ec.g, ec.b);
}

void Image::mask_blit_dst(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b) {

  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint64_t _r, _g, _b, _a;
      this->read_pixel(x + xx, y + yy, &_r, &_g, &_b, &_a);
      if (r == _r && g == _g && b == _b) {
        source.read_pixel(sx + xx, sy + yy, &_r, &_g, &_b, &_a);
        this->write_pixel(x + xx, y + yy, _r, _g, _b, _a);
      }
    }
  }
}
void Image::mask_blit_dst(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, uint32_t transparent_c) {
  auto ec = expand_color(transparent_c);
  this->mask_blit_dst(source, x, y, w, h, sx, sy, ec.r, ec.g, ec.b);
}

void Image::mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, const Image& mask) {
  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  // The mask image must cover the entire area to be blitted, but it does NOT
  // have to cover the entire destination or source image. The mask is indexed
  // in source-space, though, so its upper-left corner must be aligned with the
  // source image's upper-left corner.
  if ((mask.get_width() < static_cast<size_t>(w)) ||
      (mask.get_height() < static_cast<size_t>(h))) {
    throw runtime_error("mask is too small to cover copied area");
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (ssize_t yy = 0; yy < h; yy++) {
    for (ssize_t xx = 0; xx < w; xx++) {
      uint64_t r, g, b, a;
      mask.read_pixel(sx + xx, sy + yy, &r, &g, &b);
      if (r == 0xFF && g == 0xFF && b == 0xFF) {
        continue;
      }
      source.read_pixel(sx + xx, sy + yy, &r, &g, &b, &a);
      this->write_pixel(x + xx, y + yy, r, g, b, a);
    }
  }
}

void Image::blend_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy) {
  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint64_t sr, sg, sb, sa;
      source.read_pixel(sx + xx, sy + yy, &sr, &sg, &sb, &sa);
      if (sa == this->max_value) {
        this->write_pixel(x + xx, y + yy, sr, sg, sb, sa);
      } else if (sa != 0x00) {
        uint64_t dr, dg, db, da;
        this->read_pixel(x + xx, y + yy, &dr, &dg, &db, &da);
        this->write_pixel(x + xx, y + yy,
            (sr * sa + dr * (this->max_value - sa)) / this->max_value,
            (sg * sa + dg * (this->max_value - sa)) / this->max_value,
            (sb * sa + db * (this->max_value - sa)) / this->max_value,
            (sa * sa + da * (this->max_value - sa)) / this->max_value);
      }
    }
  }
}

void Image::blend_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, uint64_t source_alpha) {
  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint64_t sr, sg, sb, sa;
      source.read_pixel(sx + xx, sy + yy, &sr, &sg, &sb, &sa);

      uint64_t effective_alpha = (source_alpha * sa) / this->max_value;
      if (effective_alpha == this->max_value) {
        this->write_pixel(x + xx, y + yy, sr, sg, sb, effective_alpha);
      } else if (effective_alpha != 0x00) {
        uint64_t dr, dg, db, da;
        this->read_pixel(x + xx, y + yy, &dr, &dg, &db, &da);
        this->write_pixel(x + xx, y + yy,
            (sr * effective_alpha + dr * (this->max_value - effective_alpha)) / this->max_value,
            (sg * effective_alpha + dg * (this->max_value - effective_alpha)) / this->max_value,
            (sb * effective_alpha + db * (this->max_value - effective_alpha)) / this->max_value,
            da);
      }
    }
  }
}

void Image::custom_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy,
    function<void(uint32_t&, uint32_t)> per_pixel_fn) {
  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint32_t sc = source.read_pixel(sx + xx, sy + yy);
      uint32_t dc = this->read_pixel(x + xx, y + yy);
      per_pixel_fn(dc, sc);
      this->write_pixel(x + xx, y + yy, dc);
    }
  }
}

void Image::custom_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy,
    function<void(uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t, uint64_t, uint64_t, uint64_t)> per_pixel_fn) {
  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  clamp_blit_dimensions(*this, source, &x, &y, &w, &h, &sx, &sy);

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint64_t sr, sg, sb, sa, dr, dg, db, da;
      source.read_pixel(sx + xx, sy + yy, &sr, &sg, &sb, &sa);
      this->read_pixel(x + xx, y + yy, &dr, &dg, &db, &da);

      per_pixel_fn(dr, dg, db, da, sr, sg, sb, sa);

      this->write_pixel(x + xx, y + yy, dr, dg, db, da);
    }
  }
}

void Image::resize_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, ssize_t sw, ssize_t sh) {
  if (w <= 0 || h <= 0) {
    throw invalid_argument("source width/height must be specified");
  }
  if (sw < 0) {
    sw = source.get_width();
  }
  if (sh < 0) {
    sh = source.get_height();
  }

  for (ssize_t yy = 0; yy < h; yy++) {
    double y_rel_dist = static_cast<double>(yy) / (h - 1);
    double source_y_progress = y_rel_dist * (sh - 1);
    size_t source_y1 = sy + source_y_progress;
    size_t source_y2 = source_y1 + 1;
    double source_y2_factor = source_y_progress - source_y1;
    double source_y1_factor = 1.0 - source_y2_factor;

    for (ssize_t xx = 0; xx < w; xx++) {
      double x_rel_dist = static_cast<double>(xx) / (w - 1);
      double source_x_progress = x_rel_dist * (sw - 1);
      size_t source_x1 = sx + source_x_progress;
      size_t source_x2 = source_x1 + 1;
      double source_x2_factor = source_x_progress - source_x1;
      double source_x1_factor = 1.0 - source_x2_factor;

      uint64_t sr11 = 0, sg11 = 0, sb11 = 0, sa11 = 0;
      uint64_t sr12 = 0, sg12 = 0, sb12 = 0, sa12 = 0;
      uint64_t sr21 = 0, sg21 = 0, sb21 = 0, sa21 = 0;
      uint64_t sr22 = 0, sg22 = 0, sb22 = 0, sa22 = 0;
      source.read_pixel(source_x1, source_y1, &sr11, &sg11, &sb11, &sa11);
      if (source_y2_factor != 0.0) {
        source.read_pixel(source_x1, source_y2, &sr12, &sg12, &sb12, &sa12);
      }
      if (source_x2_factor != 0.0) {
        source.read_pixel(source_x2, source_y1, &sr21, &sg21, &sb21, &sa21);
      }
      if (source_x2_factor != 0.0 && source_y2_factor != 0.0) {
        source.read_pixel(source_x2, source_y2, &sr22, &sg22, &sb22, &sa22);
      }

      uint64_t dr = sr11 * (source_x1_factor * source_y1_factor) +
          sr12 * (source_x1_factor * source_y2_factor) +
          sr21 * (source_x2_factor * source_y1_factor) +
          sr22 * (source_x2_factor * source_y2_factor);
      uint64_t dg = sg11 * (source_x1_factor * source_y1_factor) +
          sg12 * (source_x1_factor * source_y2_factor) +
          sg21 * (source_x2_factor * source_y1_factor) +
          sg22 * (source_x2_factor * source_y2_factor);
      uint64_t db = sb11 * (source_x1_factor * source_y1_factor) +
          sb12 * (source_x1_factor * source_y2_factor) +
          sb21 * (source_x2_factor * source_y1_factor) +
          sb22 * (source_x2_factor * source_y2_factor);
      uint64_t da = sa11 * (source_x1_factor * source_y1_factor) +
          sa12 * (source_x1_factor * source_y2_factor) +
          sa21 * (source_x2_factor * source_y1_factor) +
          sa22 * (source_x2_factor * source_y2_factor);

      this->write_pixel(x + xx, y + yy, dr, dg, db, da);
    }
  }
}
