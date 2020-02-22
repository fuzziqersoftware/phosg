#include "Image.hh"

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <stdexcept>

#include "Filesystem.hh"
#include "Strings.hh"

#ifndef WINDOWS
#include "ImageTextFont.hh"
#endif

using namespace std;


Image::unknown_format::unknown_format(const std::string& what) : runtime_error(what) { }


struct WindowsBitmapFileHeader {
  uint16_t magic;
  uint32_t file_size;
  uint16_t reserved[2];
  uint32_t data_offset;
} __attribute__((packed));

struct WindowsBitmapInfoHeader {
  uint32_t header_size;
  int32_t width;
  int32_t height;
  uint16_t num_planes;
  uint16_t bit_depth;
  uint32_t compression; // only BI_RGB (0) and BI_BITFIELDS (3) are supported
  uint32_t image_size;
  int32_t x_pixels_per_meter;
  int32_t y_pixels_per_meter;
  uint32_t num_used_colors;
  uint32_t num_important_colors;

  // the following are only present for 32-bit bitmaps apparently
  uint32_t bitmask_r;
  uint32_t bitmask_g;
  uint32_t bitmask_b;
  uint32_t bitmask_a;

  static const size_t SIZE24 = 0x28;
} __attribute__((packed));

struct WindowsBitmapHeader {
  WindowsBitmapFileHeader file_header;
  WindowsBitmapInfoHeader info_header;
} __attribute__((packed));


void Image::load(FILE* f) {
  char sig[2];

  // read signature. this will tell us what kind of file it is
  freadx(f, sig, 2);

  // find out what kind of image it is
  ImageFormat format;
  if (sig[0] == 'P' && sig[1] == '5') {
    format = GrayscalePPM;
  } else if (sig[0] == 'P' && sig[1] == '6') {
    format = ColorPPM;
  } else if (sig[0] == 'B' && sig[1] == 'M') {
    format = WindowsBitmap;
  } else {
    throw unknown_format(string_printf(
        "can\'t load image; type signature is %02X%02X", sig[0], sig[1]));
  }

  if (format == GrayscalePPM || format == ColorPPM) {
    size_t new_width, new_height;
    uint64_t new_max_value;
    if (fscanf(f, "%zu", &new_width) != 1) {
      throw runtime_error("cannot read width field in PPM header");
    }
    if (fscanf(f, "%zu", &new_height) != 1) {
      throw runtime_error("cannot read height field in PPM header");
    }
    if (fscanf(f, "%" PRIu64, &new_max_value) != 1) {
      throw runtime_error("cannot read max value field in PPM header");
    }

    // according to the docs, this can be any whitespace character but is
    // "usually" a newline. guess we shouldn't make assumptions here
    char header_end_char = fgetc(f);
    if ((header_end_char != ' ') && ((header_end_char != '\t') &&
        (header_end_char != '\n'))) {
      throw runtime_error("whitespace character not present after PPM header");
    }

    // the format docs say this is limited to 0xFFFF, but we'll support up to
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

    void* new_data = malloc(new_width * new_height * 3 * (new_channel_width / 8));
    if (!new_data) {
      throw bad_alloc();
    }
    freadx(f, new_data, new_width * new_height * (format == ColorPPM ? 3 : 1) * (new_channel_width / 8));

    // if the read succeeded, we can commit the changes - nothing after here can
    // throw an exception

    this->width = new_width;
    this->height = new_height;
    this->has_alpha = false;
    this->channel_width = new_channel_width;
    this->max_value = new_max_value;
    this->data = new_data;

    // expand grayscale data into color data if necessary
    if (format == GrayscalePPM) {
      if (this->channel_width == 8) {
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = this->width - 1; x >= 0; x--) {
            this->data8[(y * this->width + x) * 3 + 0] = this->data8[y * this->width + x];
            this->data8[(y * this->width + x) * 3 + 1] = this->data8[y * this->width + x];
            this->data8[(y * this->width + x) * 3 + 2] = this->data8[y * this->width + x];
          }
        }
      } else if (this->channel_width == 16) {
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = this->width - 1; x >= 0; x--) {
            this->data16[(y * this->width + x) * 3 + 0] = this->data16[y * this->width + x];
            this->data16[(y * this->width + x) * 3 + 1] = this->data16[y * this->width + x];
            this->data16[(y * this->width + x) * 3 + 2] = this->data16[y * this->width + x];
          }
        }
      } else if (this->channel_width == 32) {
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = this->width - 1; x >= 0; x--) {
            this->data32[(y * this->width + x) * 3 + 0] = this->data32[y * this->width + x];
            this->data32[(y * this->width + x) * 3 + 1] = this->data32[y * this->width + x];
            this->data32[(y * this->width + x) * 3 + 2] = this->data32[y * this->width + x];
          }
        }
      } else if (this->channel_width == 64) {
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = this->width - 1; x >= 0; x--) {
            this->data64[(y * this->width + x) * 3 + 0] = this->data64[y * this->width + x];
            this->data64[(y * this->width + x) * 3 + 1] = this->data64[y * this->width + x];
            this->data64[(y * this->width + x) * 3 + 2] = this->data64[y * this->width + x];
          }
        }
      }
    }

  } else if (format == WindowsBitmap) {
    WindowsBitmapHeader header;
    memcpy(&header.file_header.magic, sig, 2);
    freadx(f, reinterpret_cast<uint8_t*>(&header) + 2, sizeof(header) - 2);
    if (header.file_header.magic != 0x4D42) {
      throw runtime_error(string_printf("bad signature in bitmap file (%04hX)",
          header.file_header.magic));
    }
    if ((header.info_header.bit_depth != 24) && (header.info_header.bit_depth != 32)) {
      throw runtime_error(string_printf(
          "can only load 24-bit or 32-bit bitmaps (this is a %hu-bit bitmap)",
          header.info_header.bit_depth));
    }
    if (header.info_header.num_planes != 1) {
      throw runtime_error("can only load 1-plane bitmaps");
    }

    bool reverse_row_order = header.info_header.height < 0;
    fseek(f, header.file_header.data_offset, SEEK_SET);
    bool has_alpha;
    int32_t w = header.info_header.width;
    int32_t h = header.info_header.height * (reverse_row_order ? -1 : 1);
    unique_ptr<void, void(*)(void*)> new_data_unique(nullptr, free);

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
      unordered_map<uint32_t, size_t> offset_for_bitmask({
          {0xFF000000, 3}, {0x00FF0000, 2}, {0x0000FF00, 1}, {0x000000FF, 0}});
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
    this->data = new_data_unique.release();
  }
}



static const unordered_map<uint8_t, uint64_t> max_value_for_channel_width({
  {8, 0xFF},
  {16, 0xFFFF},
  {32, 0xFFFFFFFF},
  {64, 0xFFFFFFFFFFFFFFFF},
});

Image::Image(size_t x, size_t y, bool has_alpha, uint8_t channel_width) :
    width(x), height(y), has_alpha(has_alpha), channel_width(channel_width),
    max_value(max_value_for_channel_width.at(this->channel_width)) {
  size_t num_bytes = this->get_data_size();
  this->data = malloc(num_bytes);
  memset(this->data, 0, num_bytes * sizeof(uint8_t));
}

Image::Image(const Image& im) : width(im.width), height(im.height),
    has_alpha(im.has_alpha), channel_width(im.channel_width),
    max_value(im.max_value) {
  size_t num_bytes = this->get_data_size();
  this->data = malloc(num_bytes);
  memcpy(this->data, im.data, num_bytes * sizeof(uint8_t));
}

const Image& Image::operator=(const Image& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->channel_width = im.channel_width;
  this->max_value = im.max_value;
  size_t num_bytes = this->get_data_size();
  if (this->data) {
    free(this->data);
  }
  this->data = malloc(num_bytes);
  memcpy(this->data, im.data, num_bytes * sizeof(uint8_t));
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
  im.data = NULL;
}

const Image& Image::operator=(Image&& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->channel_width = im.channel_width;
  this->max_value = im.max_value;
  if (this->data) {
    free(this->data);
  }
  this->data = im.data;

  im.width = 0;
  im.height = 0;
  im.has_alpha = false;
  im.channel_width = 8;
  im.max_value = 0xFF;
  im.data = NULL;
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

Image::Image(const string& filename) : Image(filename.c_str()) { }

Image::Image(FILE* f, ssize_t width, ssize_t height, bool has_alpha,
    uint8_t channel_width, uint64_t max_value) : width(width), height(height),
    has_alpha(has_alpha), channel_width(channel_width),
    max_value(max_value ? max_value : max_value_for_channel_width.at(this->channel_width)) {
  size_t num_bytes = this->get_data_size();
  this->data = malloc(num_bytes);
  freadx(f, this->data, num_bytes);
}

Image::Image(const char* filename, ssize_t width, ssize_t height,
    bool has_alpha, uint8_t channel_width, uint64_t max_value) : width(width),
    height(height), has_alpha(has_alpha), channel_width(channel_width),
    max_value(max_value ? max_value : max_value_for_channel_width.at(this->channel_width)) {
  auto f = fopen_unique(filename, "rb");
  size_t num_bytes = this->get_data_size();
  this->data = malloc(num_bytes);
  freadx(f.get(), this->data, num_bytes);
}

Image::Image(const std::string& filename, ssize_t width, ssize_t height,
    bool has_alpha, uint8_t channel_width, uint64_t max_value) :
    Image(filename.c_str(), width, height, has_alpha, channel_width, max_value) { }

Image::~Image() {
  if (this->channel_width == 8) {
    delete[] this->data8;
  } else if (this->channel_width == 16) {
    delete[] this->data16;
  } else if (this->channel_width == 32) {
    delete[] this->data32;
  } else if (this->channel_width == 64) {
    delete[] this->data64;
  }
}

bool Image::operator==(const Image& other) {
  if ((this->width != other.width) || (this->height != other.height) ||
      (this->has_alpha != other.has_alpha) ||
      (this->channel_width != other.channel_width) ||
      (this->max_value != other.max_value)) {
    return false;
  }
  return !memcmp(this->data, other.data, this->get_data_size());
}

const char* Image::mime_type_for_format(ImageFormat format) {
  switch (format) {
    case GrayscalePPM:
    case ColorPPM:
      return "image/x-portable-pixmap";
    case WindowsBitmap:
      return "image/bmp";
    default:
      return "text/plain";
  }
}

const char* Image::file_extension_for_format(ImageFormat format) {
  switch (format) {
    case GrayscalePPM:
    case ColorPPM:
      return "ppm";
    case WindowsBitmap:
      return "bmp";
    default:
      return "raw";
  }
}

// save the image to an already-open file
void Image::save(FILE* f, Image::ImageFormat format) const {

  switch (format) {
    case GrayscalePPM:
      throw runtime_error("can\'t save grayscale ppm files");

    case ColorPPM:
      if (this->has_alpha) {
        throw runtime_error("can\'t save color ppm with alpha");
      }
      fprintf(f, "P6 %zu %zu %" PRIu64 "\n", this->width, this->height,
          this->max_value);
      fwritex(f, this->data, this->get_data_size());
      break;

    case WindowsBitmap: {
      if (this->channel_width != 8) {
        throw runtime_error("can\'t save bmp with more than 8-bit channels");
      }

      size_t pixel_bytes = 3 + this->has_alpha;
      size_t row_padding_bytes = (4 - ((this->width * pixel_bytes) % 4)) % 4;
      uint8_t row_padding_data[4] = {0, 0, 0, 0};

      size_t header_size = sizeof(WindowsBitmapFileHeader) + (this->has_alpha ? sizeof(WindowsBitmapInfoHeader) : WindowsBitmapInfoHeader::SIZE24);

      WindowsBitmapHeader header;
      header.file_header.magic = 0x4D42; // 'BM'
      header.file_header.file_size = header_size + (this->width * this->height * pixel_bytes) + (row_padding_bytes * this->height);
      header.file_header.reserved[0] = 0;
      header.file_header.reserved[1] = 0;
      header.file_header.data_offset = header_size;
      header.info_header.header_size = header_size - sizeof(WindowsBitmapFileHeader);
      header.info_header.width = this->width;
      header.info_header.height = this->height;
      header.info_header.num_planes = 1;
      header.info_header.bit_depth = this->has_alpha ? 32 : 24;
      header.info_header.compression = this->has_alpha ? 3 : 0; // BI_BITFIELDS or BI_RGB
      header.info_header.image_size = this->has_alpha ? (this->width * this->height * 4) : 0; // 0 acceptable for BI_RGB
      header.info_header.x_pixels_per_meter = 0x00000B12;
      header.info_header.y_pixels_per_meter = 0x00000B12;
      header.info_header.num_used_colors = 0;
      header.info_header.num_important_colors = 0;
      if (this->has_alpha) {
        header.info_header.bitmask_r = 0x000000FF;
        header.info_header.bitmask_g = 0x0000FF00;
        header.info_header.bitmask_b = 0x00FF0000;
        header.info_header.bitmask_a = 0xFF000000;
      }
      fwrite(&header, header_size, 1, f);

      if (this->has_alpha) {
        // there's no padding and the bitmasks already specify how to read each
        // pixel; just write each row
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          fwrite(&this->data8[y * this->width * 4], this->width * 4, 1, f);
        }

      } else {
        auto row_data_unique = malloc_unique(this->width * 3);
        uint8_t* row_data = reinterpret_cast<uint8_t*>(row_data_unique.get());
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = 0; x < this->width * 3; x += 3) {
            row_data[x] = this->data8[y * this->width * 3 + x + 2];
            row_data[x + 1] = this->data8[y * this->width * 3 + x + 1];
            row_data[x + 2] = this->data8[y * this->width * 3 + x];
          }
          fwrite(row_data, this->width * 3, 1, f);
          if (row_padding_bytes) {
            fwrite(row_padding_data, row_padding_bytes, 1, f);
          }
        }
      }

      break;
    }

    default:
      throw runtime_error("unknown file format in Image::save()");
  }
}

// save the image to a string in memory
string Image::save(Image::ImageFormat format) const {

  // TODO: deduplicate this implementation with Image::save(FILE*)

  switch (format) {
    case GrayscalePPM:
      throw runtime_error("can\'t save grayscale ppm files");

    case ColorPPM: {
      if (this->has_alpha) {
        throw runtime_error("can\'t save color ppm with alpha");
      }
      string s = string_printf("P6 %d %d %" PRIu64 "\n", this->width,
          this->height, this->max_value);
      s.append((const char*)this->data, this->get_data_size());
      return s;
    }

    case WindowsBitmap: {
      if (this->channel_width != 8) {
        throw runtime_error("can\'t save bmp with more than 8-bit channels");
      }

      size_t pixel_bytes = 3 + this->has_alpha;
      size_t row_padding_bytes = (4 - ((this->width * pixel_bytes) % 4)) % 4;
      char row_padding_data[4] = {0, 0, 0, 0};

      size_t header_size = sizeof(WindowsBitmapFileHeader) + (this->has_alpha ? sizeof(WindowsBitmapInfoHeader) : WindowsBitmapInfoHeader::SIZE24);

      WindowsBitmapHeader header;
      header.file_header.magic = 0x4D42; // 'BM'
      header.file_header.file_size = header_size + (this->width * this->height * pixel_bytes) + (row_padding_bytes * this->height);
      header.file_header.reserved[0] = 0;
      header.file_header.reserved[1] = 0;
      header.file_header.data_offset = header_size;
      header.info_header.header_size = header_size - sizeof(WindowsBitmapFileHeader);
      header.info_header.width = this->width;
      header.info_header.height = this->height;
      header.info_header.num_planes = 1;
      header.info_header.bit_depth = this->has_alpha ? 32 : 24;
      header.info_header.compression = this->has_alpha ? 3 : 0; // BI_BITFIELDS or BI_RGB
      header.info_header.image_size = this->has_alpha ? (this->width * this->height * 4) : 0; // 0 acceptable for BI_RGB
      header.info_header.x_pixels_per_meter = 0x00000B12;
      header.info_header.y_pixels_per_meter = 0x00000B12;
      header.info_header.num_used_colors = 0;
      header.info_header.num_important_colors = 0;
      if (this->has_alpha) {
        header.info_header.bitmask_r = 0x000000FF;
        header.info_header.bitmask_g = 0x0000FF00;
        header.info_header.bitmask_b = 0x00FF0000;
        header.info_header.bitmask_a = 0xFF000000;
      }

      string s;
      s.append((const char*)&header, header_size);

      if (this->has_alpha) {
        // there's no padding and the bitmasks already specify how to read each
        // pixel; just write each row
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          s.append((const char*)&this->data8[y * this->width * 4], this->width * 4);
        }

      } else {
        auto row_data_unique = malloc_unique(this->width * 3);
        uint8_t* row_data = reinterpret_cast<uint8_t*>(row_data_unique.get());
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = 0; x < this->width * 3; x += 3) {
            row_data[x] = this->data8[y * this->width * 3 + x + 2];
            row_data[x + 1] = this->data8[y * this->width * 3 + x + 1];
            row_data[x + 2] = this->data8[y * this->width * 3 + x];
          }
          s.append((const char*)row_data, this->width * 3);
          if (row_padding_bytes) {
            s.append(row_padding_data, row_padding_bytes);
          }
        }
      }

      return s;
    }

    default:
      throw runtime_error("unknown file format in Image::save()");
  }
}

// saves the Image. if NULL is given for filename, writes to stdout
void Image::save(const char* filename, Image::ImageFormat format) const {
  if (filename) {
    auto f = fopen_unique(filename, "wb");
    this->save(f.get(), format);
  } else {
    this->save(stdout, format);
  }
}

// saves the Image
void Image::save(const string& filename, Image::ImageFormat format) const {
  auto f = fopen_unique(filename, "wb");
  this->save(f.get(), format);
}

// fill the entire image with this color
void Image::clear(uint64_t r, uint64_t g, uint64_t b, uint64_t a) {
  for (ssize_t y = 0; y < this->height; y++) {
    for (ssize_t x = 0; x < this->width; x++) {
      this->write_pixel(x, y, r, g, b, a);
    }
  }
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
      *r = this->data8[index];
    }
    if (g) {
      *g = this->data8[index + 1];
    }
    if (b) {
      *b = this->data8[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data8[index + 3] : this->max_value;
    }
  } else if (this->channel_width == 16) {
    if (r) {
      *r = this->data16[index];
    }
    if (g) {
      *g = this->data16[index + 1];
    }
    if (b) {
      *b = this->data16[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data16[index + 3] : this->max_value;
    }
  } else if (this->channel_width == 32) {
    if (r) {
      *r = this->data32[index];
    }
    if (g) {
      *g = this->data32[index + 1];
    }
    if (b) {
      *b = this->data32[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data32[index + 3] : this->max_value;
    }
  } else if (this->channel_width == 64) {
    if (r) {
      *r = this->data64[index];
    }
    if (g) {
      *g = this->data64[index + 1];
    }
    if (b) {
      *b = this->data64[index + 2];
    }
    if (a) {
      *a = this->has_alpha ? this->data64[index + 3] : this->max_value;
    }
  } else {
    throw logic_error("image channel width is not 8, 16, 32, or 64");
  }
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
    this->data8[index] = r;
    this->data8[index + 1] = g;
    this->data8[index + 2] = b;
    if (this->has_alpha) {
      this->data8[index + 3] = a;
    }
  } else if (this->channel_width == 16) {
    this->data16[index] = r;
    this->data16[index + 1] = g;
    this->data16[index + 2] = b;
    if (this->has_alpha) {
      this->data16[index + 3] = a;
    }
  } else if (this->channel_width == 32) {
    this->data32[index] = r;
    this->data32[index + 1] = g;
    this->data32[index + 2] = b;
    if (this->has_alpha) {
      this->data32[index + 3] = a;
    }
  } else if (this->channel_width == 64) {
    this->data64[index] = r;
    this->data64[index + 1] = g;
    this->data64[index + 2] = b;
    if (this->has_alpha) {
      this->data64[index + 3] = a;
    }
  } else {
    throw logic_error("image channel width is not 8, 16, 32, or 64");
  }
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

#ifndef WINDOWS
void Image::draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
    uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
    uint64_t bb, uint64_t ba, const char* fmt, ...) {

  char* buffer;
  va_list va;
  va_start(va, fmt);
  vasprintf(&buffer, fmt, va);
  va_end(va);
  if (!buffer) {
    throw bad_alloc();
  }

  ssize_t max_x_pos = 0;
  ssize_t x_pos = x, y_pos = y;
  for (ssize_t z = 0; buffer[z]; z++) {
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
        } catch (const runtime_error& e) { }
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

  free(buffer);
}
#endif

void Image::blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy) {

  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  for (ssize_t yy = 0; yy < h; yy++) {
    for (ssize_t xx = 0; xx < w; xx++) {
      try {
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
      } catch (const runtime_error& e) { }
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

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      try {
        uint64_t _r, _g, _b, _a;
        source.read_pixel(sx + xx, sy + yy, &_r, &_g, &_b, &_a);
        if (r != _r || g != _g || b != _b) {
          this->write_pixel(x + xx, y + yy, _r, _g, _b, _a);
        }
      } catch (const runtime_error& e) { }
    }
  }
}

void Image::mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
    ssize_t h, ssize_t sx, ssize_t sy, const Image& mask) {

  if ((source.get_width() != mask.get_width()) || (source.get_height() != mask.get_height())) {
    throw runtime_error("mask dimensions don\'t match image dimensions");
  }

  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  for (ssize_t yy = 0; yy < h; yy++) {
    for (ssize_t xx = 0; xx < w; xx++) {
      try {
        uint64_t r, g, b, a;
        mask.read_pixel(sx + xx, sy + yy, &r, &g, &b);
        if (r == 0xFF && g == 0xFF && b == 0xFF) {
          continue;
        }
        source.read_pixel(sx + xx, sy + yy, &r, &g, &b, &a);
        this->write_pixel(x + xx, y + yy, r, g, b, a);
      } catch (const runtime_error& e) { }
    }
  }
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
        } catch (const runtime_error& e) { }
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
        } catch (const runtime_error& e) { }
      }
    }
  }
}
