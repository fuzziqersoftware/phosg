#pragma once

#include <stdint.h>
#include <stdio.h>

#include <stdexcept>
#include <string>


// an Image represents a drawing canvas. this class is fairly simple; it
// supports reading/writing individual pixels, drawing lines, and saving the
// image as a PPM or Windows BMP file.
class Image {
private:
  ssize_t width;
  ssize_t height;
  bool has_alpha;
  uint8_t channel_width;
  uint64_t max_value;
  union {
    void* data;
    uint8_t* data8;
    uint16_t* data16;
    uint32_t* data32;
    uint64_t* data64;
  };

  void load(FILE* f);

public:
  Image() = delete;

  // construct a black-filled Image in memory
  Image(size_t x, size_t y, bool has_alpha = false, uint8_t channel_width = 8);

  // copy/move from another image
  Image(const Image&);
  Image(Image&&);

  // load a file (autodetect format)
  Image(FILE* f);
  Image(const char* filename);
  Image(const std::string& filename);

  // load from a file (raw data)
  Image(FILE* f, ssize_t width, ssize_t height, bool has_alpha = false,
      uint8_t channel_width = 0, uint64_t max_value = 0);
  Image(const char* filename, ssize_t width, ssize_t height,
      bool has_alpha = false, uint8_t channel_width = 0,
      uint64_t max_value = 0);
  Image(const std::string& filename, ssize_t width, ssize_t height,
      bool has_alpha = false, uint8_t channel_width = 0,
      uint64_t max_value = 0);

  ~Image();

  const Image& operator=(const Image& other);
  const Image& operator=(Image&& other);

  bool operator==(const Image& other);

  inline size_t get_width() const {
    return this->width;
  }
  inline size_t get_height() const {
    return this->height;
  }
  inline bool get_has_alpha() const {
    return this->has_alpha;
  }
  inline const void* get_data() const {
    return this->data;
  }
  inline uint8_t get_channel_width() const {
    return this->channel_width;
  }
  inline size_t get_data_size() const {
    return this->width * this->height * (3 + this->has_alpha) * (this->channel_width / 8);
  }

  enum ImageFormat {
    GrayscalePPM = 0,
    ColorPPM = 1,
    WindowsBitmap = 2,
  };

  class unknown_format : virtual public std::runtime_error {
  public:
    unknown_format(const std::string& what);
  };

  static const char* mime_type_for_format(ImageFormat format);
  static const char* file_extension_for_format(ImageFormat format);

  void save(FILE* f, ImageFormat format) const;
  void save(const char* filename, ImageFormat format) const;
  void save(const std::string& filename, ImageFormat format) const;
  std::string save(ImageFormat format) const;

  // read/write functions
  void clear(uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void read_pixel(ssize_t x, ssize_t y, uint64_t* r, uint64_t* g, uint64_t* b, uint64_t* a = NULL) const;
  void write_pixel(ssize_t x, ssize_t y, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);

  // drawing functions
  // note: no drawing functions respect the alpha channel - they all set it to
  // the given alpha value and ignore whatever's already in the image
  void draw_line(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2, uint64_t r,
      uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y,
      ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b,
      uint64_t a = 0xFF);
  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2,
      ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b,
      uint64_t a = 0xFF);
#ifndef WINDOWS
  void draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
      uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
      uint64_t bb, uint64_t ba, const char* fmt, ...);
#endif
  void fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint64_t r,
      uint64_t g, uint64_t b, uint64_t a = 0xFF);

  // copy functions
  // this doesn't respect alpha; the written pixels will have the same alpha as
  // in the source image
  void blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, const Image& mask);
};
