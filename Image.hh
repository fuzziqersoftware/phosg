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
  int width;
  int height;
  uint8_t* data;

  void load(FILE* f);

public:
  Image() = delete;
  Image(int x, int y);
  Image(const Image&);
  Image(FILE* f);
  Image(const char* filename);
  Image(const std::string& filename);
  ~Image();

  const Image& operator=(const Image& other);

  inline int get_width() const {
    return this->width;
  }
  inline int get_height() const {
    return this->height;
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
  void clear(uint8_t r, uint8_t g, uint8_t b);
  void read_pixel(int x, int y, uint8_t* r, uint8_t* g, uint8_t* b) const;
  void write_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

  // drawing functions
  void draw_line(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
  void draw_horizontal_line(int x1, int x2, int y, int dash_length, uint8_t r, uint8_t g, uint8_t b);
  void draw_vertical_line(int x, int y1, int y2, int dash_length, uint8_t r, uint8_t g, uint8_t b);
  void draw_text(int x, int y, int* width, int* height, uint8_t r, uint8_t g,
      uint8_t b, uint8_t br, uint8_t bg, uint8_t bb, uint8_t ba, const char* fmt, ...);

  void blit(const Image& source, int x, int y, int w, int h, int sx, int sy);
  void mask_blit(const Image& source, int x, int y, int w, int h, int sx, int sy, uint8_t r, uint8_t g, uint8_t b);
  void mask_blit(const Image& source, int x, int y, int w, int h, int sx, int sy, const Image& mask);

  void fill_rect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
};
