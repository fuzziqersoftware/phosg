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
  uint8_t* data;

  void load(FILE* f);

public:
  Image() = delete;
  Image(size_t x, size_t y, bool has_alpha = false);
  Image(const Image&);
  Image(Image&&);
  Image(FILE* f);
  Image(const char* filename);
  Image(const std::string& filename);
  ~Image();

  const Image& operator=(const Image& other);
  const Image& operator=(Image&& other);

  inline size_t get_width() const {
    return this->width;
  }
  inline size_t get_height() const {
    return this->height;
  }
  inline bool get_has_alpha() const {
    return this->has_alpha;
  }
  inline const uint8_t* get_data() const {
    return this->data;
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

  void save(FILE* f, ImageFormat format) const;
  void save(const char* filename, ImageFormat format) const;
  std::string save(ImageFormat format) const;

  // read/write functions
  void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
  void read_pixel(ssize_t x, ssize_t y, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a = NULL) const;
  void write_pixel(ssize_t x, ssize_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);

  // drawing functions
  // note: no drawing functions respect the alpha channel - they all set it to
  // the given alpha value and ignore whatever's already in the image
  void draw_line(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2, uint8_t r,
      uint8_t g, uint8_t b, uint8_t a = 0xFF);
  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y,
      ssize_t dash_length, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2,
      ssize_t dash_length, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);
  void draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
      uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t br, uint8_t bg,
      uint8_t bb, uint8_t ba, const char* fmt, ...);
  void fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint8_t r,
      uint8_t g, uint8_t b, uint8_t a = 0xFF);

  // copy functions
  // this doesn't respect alpha; the written pixels will have the same alpha as
  // in the source image
  void blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, uint8_t r, uint8_t g, uint8_t b);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, const Image& mask);
};
