#include "Image.hh"

#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <stdexcept>

#include "Filesystem.hh"
#include "ImageTextFont.hh"
#include "Strings.hh"

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
  fread(sig, 2, 1, f);

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
    unsigned char color_max = 0;
    fscanf(f, "%zu", &this->width);
    fscanf(f, "%zu", &this->height);
    fscanf(f, "%hhu", &color_max);
    fgetc(f); // skip the newline

    this->has_alpha = false;
    this->data = new uint8_t[width * height * 3];
    fread(this->data, width * height * (format == ColorPPM ? 3 : 1), 1, f);

    // expand grayscale data into color data if necessary
    if (format == GrayscalePPM) {
      for (ssize_t y = this->height - 1; y >= 0; y--) {
        for (ssize_t x = this->width - 1; x >= 0; x--) {
          this->data[(y * this->width + x) * 3 + 0] = this->data[y * this->width + x];
          this->data[(y * this->width + x) * 3 + 1] = this->data[y * this->width + x];
          this->data[(y * this->width + x) * 3 + 2] = this->data[y * this->width + x];
        }
      }
    }

  } else if (format == WindowsBitmap) {
    WindowsBitmapHeader header;
    memcpy(&header.file_header.magic, sig, 2);
    fread(reinterpret_cast<uint8_t*>(&header) + 2, sizeof(header) - 2, 1, f);
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
    uint8_t* new_data = new uint8_t[w * h * 3];

    if (header.info_header.compression == 0) { // BI_RGB
      if (header.info_header.bit_depth != 24) {
        throw runtime_error("bitmap uses BI_RGB but bit depth is not 24");
      }

      has_alpha = false;
      size_t row_padding_bytes = (4 - ((w * 3) % 4)) % 4;

      uint8_t* row_data = new uint8_t[w * 3];
      for (int32_t y = h - 1; y >= 0; y--) {
        fread(row_data, w * 3, 1, f);
        ssize_t target_y = reverse_row_order ? (h - y - 1) : y;
        ssize_t target_y_offset = target_y * w * 3;
        for (int32_t x = 0; x < w; x++) {
          size_t x_offset = x * 3;
          new_data[target_y_offset + x_offset + 2] = row_data[x_offset + 0];
          new_data[target_y_offset + x_offset + 1] = row_data[x_offset + 1];
          new_data[target_y_offset + x_offset + 0] = row_data[x_offset + 2];
        }
        if (row_padding_bytes) {
          fseek(f, row_padding_bytes, SEEK_CUR);
        }
      }
      delete[] row_data;

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

      uint8_t* row_data = new uint8_t[w * 4];
      for (int32_t y = h - 1; y >= 0; y--) {
        fread(row_data, w * 4, 1, f);
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
      delete[] row_data;

    } else {
      throw runtime_error("can only load uncompressed or bitfield bitmaps");
    }

    // load was successful; commit the changes
    this->width = w;
    this->height = h;
    this->has_alpha = has_alpha;
    this->data = new_data;
  }
}



Image::Image(size_t x, size_t y, bool has_alpha) {
  this->width = x;
  this->height = y;
  this->has_alpha = has_alpha;
  size_t num_bytes = this->width * this->height * (3 + this->has_alpha);
  this->data = new uint8_t[num_bytes];
  memset(this->data, 0, num_bytes * sizeof(uint8_t));
}

Image::Image(const Image& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  size_t num_bytes = this->width * this->height * (3 + this->has_alpha);
  this->data = new uint8_t[num_bytes];
  memcpy(this->data, im.data, num_bytes * sizeof(uint8_t));
}

const Image& Image::operator=(const Image& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  size_t num_bytes = this->width * this->height * (3 + this->has_alpha);
  this->data = new uint8_t[num_bytes];
  memcpy(this->data, im.data, num_bytes * sizeof(uint8_t));
  return *this;
}

Image::Image(Image&& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->data = im.data;

  im.width = 0;
  im.height = 0;
  im.has_alpha = false;
  im.data = NULL;
}

const Image& Image::operator=(Image&& im) {
  this->width = im.width;
  this->height = im.height;
  this->has_alpha = im.has_alpha;
  this->data = im.data;

  im.width = 0;
  im.height = 0;
  im.has_alpha = false;
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

Image::~Image() {
  delete[] this->data;
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

// save the image to an already-open file
void Image::save(FILE* f, Image::ImageFormat format) const {

  switch (format) {
    case GrayscalePPM:
      throw runtime_error("can\'t save grayscale ppm files");

    case ColorPPM:
      if (this->has_alpha) {
        throw runtime_error("cannot save color ppm with alpha");
      }
      fprintf(f, "P6 %zu %zu 255\n", this->width, this->height);
      fwrite(this->data, this->width * this->height * 3, 1, f);
      break;

    case WindowsBitmap: {

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
          fwrite(&this->data[y * this->width * 4], this->width * 4, 1, f);
        }

      } else {
        uint8_t* row_data = new uint8_t[this->width * 3];
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = 0; x < this->width * 3; x += 3) {
            row_data[x] = this->data[y * this->width * 3 + x + 2];
            row_data[x + 1] = this->data[y * this->width * 3 + x + 1];
            row_data[x + 2] = this->data[y * this->width * 3 + x];
          }
          fwrite(row_data, this->width * 3, 1, f);
          if (row_padding_bytes) {
            fwrite(row_padding_data, row_padding_bytes, 1, f);
          }
        }
        delete[] row_data;
      }

      break;
    }

    default:
      throw runtime_error("unknown file format in Image::save()");
  }
}

// save the image to a string in memory
string Image::save(Image::ImageFormat format) const {

  switch (format) {
    case GrayscalePPM:
      throw runtime_error("can\'t save grayscale ppm files");

    case ColorPPM: {
      string s = string_printf("P6 %d %d 255\n", width, height);
      s.append((const char*)this->data, this->width * this->height * 3);
      return s;
    }

    case WindowsBitmap: {
      // TODO: deduplicate this implementation with Image::save(FILE*)

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
          s.append((const char*)&this->data[y * this->width * 4], this->width * 4);
        }

      } else {
        uint8_t* row_data = new uint8_t[this->width * 3];
        for (ssize_t y = this->height - 1; y >= 0; y--) {
          for (ssize_t x = 0; x < this->width * 3; x += 3) {
            row_data[x] = this->data[y * this->width * 3 + x + 2];
            row_data[x + 1] = this->data[y * this->width * 3 + x + 1];
            row_data[x + 2] = this->data[y * this->width * 3 + x];
          }
          s.append((const char*)row_data, this->width * 3);
          if (row_padding_bytes) {
            s.append(row_padding_data, row_padding_bytes);
          }
        }
        delete[] row_data;
      }

      return s;
    }

    default:
      throw runtime_error("unknown file format in Image::save()");
  }
}

// saves the Image as a PPM (P6) file. if NULL is given, writes to stdout
void Image::save(const char* filename, Image::ImageFormat format) const {
  if (filename) {
    auto f = fopen_unique(filename, "wb");
    this->save(f.get(), format);
  } else {
    this->save(stdout, format);
  }
}

// fill the entire image with this color
void Image::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (this->has_alpha) {
    for (size_t x = 0; x < this->width * this->height; x++) {
      this->data[x * 4 + 0] = r;
      this->data[x * 4 + 1] = g;
      this->data[x * 4 + 2] = b;
      this->data[x * 4 + 3] = a;
    }
  } else {
    for (size_t x = 0; x < this->width * this->height; x++) {
      this->data[x * 3 + 0] = r;
      this->data[x * 3 + 1] = g;
      this->data[x * 3 + 2] = b;
    }
  }
}

// read the specified pixel's rgb values
void Image::read_pixel(ssize_t x, ssize_t y, uint8_t* r, uint8_t* g, uint8_t* b,
    uint8_t* a) const {

  // check coordinates
  if (x < 0 || y < 0 || x >= this->width || y >= this->height) {
    throw runtime_error("out of bounds");
  }

  // read multiple channels
  size_t index = (y * this->width + x) * (this->has_alpha ? 4 : 3);
  if (r) {
    *r = this->data[index];
  }
  if (g) {
    *g = this->data[index + 1];
  }
  if (b) {
    *b = this->data[index + 2];
  }
  if (a) {
    *a = this->has_alpha ? this->data[index + 3] : 0xFF;
  }
}

// write the specified pixel's rgb values
void Image::write_pixel(ssize_t x, ssize_t y, uint8_t r, uint8_t g, uint8_t b,
    uint8_t a) {

  // check coordinates
  if (x < 0 || y < 0 || x >= this->width || y >= this->height) {
    throw runtime_error("out of bounds");
  }

  // write channels
  size_t index = (y * this->width + x) * (this->has_alpha ? 4 : 3);
  this->data[index] = r;
  this->data[index + 1] = g;
  this->data[index + 2] = b;
  if (this->has_alpha) {
    this->data[index + 3] = a;
  }
}

// use the Bresenham algorithm to draw a line between the specified points
void Image::draw_line(ssize_t x0, ssize_t y0, ssize_t x1, ssize_t y1, uint8_t r,
    uint8_t g, uint8_t b, uint8_t a) {

  // if both endpoints are outside the image, don't bother
  if ((x0 < 0 || x0 >= width || y0 < 0 || y0 >= height) &&
      (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height)) {
    return;
  }

  // line is too steep? then we step along y rather than x
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    y0 ^= x0;
    x0 ^= y0;
    y0 ^= x0;
    y1 ^= x1;
    x1 ^= y1;
    y1 ^= x1;
  }

  // line is backward? then switch the points
  if (x0 > x1) {
    x1 ^= x0;
    x0 ^= x1;
    x1 ^= x0;
    y1 ^= y0;
    y0 ^= y1;
    y1 ^= y0;
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
    ssize_t dash_length, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

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
    ssize_t dash_length, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

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

void Image::draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t br, uint8_t bg,
    uint8_t bb, uint8_t ba, const char* fmt, ...) {

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
        uint8_t r, g, b, a;
        source.read_pixel(sx + xx, sy + yy, &r, &g, &b, &a);
        if (a == 0) {
          continue;
        }
        if (a != 0xFF) {
          uint8_t sr, sg, sb, sa;
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
    ssize_t h, ssize_t sx, ssize_t sy, uint8_t r, uint8_t g, uint8_t b) {

  if (w < 0) {
    w = source.get_width();
  }
  if (h < 0) {
    h = source.get_height();
  }

  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      try {
        uint8_t _r, _g, _b, _a;
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
        uint8_t r, g, b, a;
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

void Image::fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint8_t r,
    uint8_t g, uint8_t b, uint8_t a) {

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > this->get_width()) {
    w = this->get_width() - x;
  }
  if (y + h > this->get_height()) {
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
          uint8_t _r = 0, _g = 0, _b = 0, _a = 0;
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
