#pragma once

#include <stdint.h>
#include <stdio.h>

#include <functional>
#include <stdexcept>
#include <string>

#include "Platform.hh"

// an Image represents a drawing canvas. this class is fairly simple; it
// supports reading/writing individual pixels, drawing lines, and saving the
// image as a PPM or Windows BMP file.
class Image {
public:
  Image();

  // construct a black-filled Image in memory
  Image(size_t x, size_t y, bool has_alpha = false, uint8_t channel_width = 8);

  // copy/move from another image
  Image(const Image&);
  Image(Image&&);

  // load a file (autodetect format)
  explicit Image(FILE* f);
  explicit Image(const char* filename);
  explicit Image(const std::string& filename);

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
  Image& operator=(Image&& other);

  bool operator==(const Image& other) const;
  bool operator!=(const Image& other) const;

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
    return this->data.raw;
  }
  inline uint8_t get_channel_width() const {
    return this->channel_width;
  }
  inline size_t get_data_size() const {
    return this->width * this->height * (3 + this->has_alpha) * (this->channel_width / 8);
  }

  inline bool empty() const {
    return (this->data.raw == nullptr);
  }

  enum class Format {
    GRAYSCALE_PPM = 0,
    COLOR_PPM,
    WINDOWS_BITMAP,
    PNG,
  };

  class unknown_format : virtual public std::runtime_error {
  public:
    unknown_format(const std::string& what);
  };

  static const char* mime_type_for_format(Format format);
  static const char* file_extension_for_format(Format format);

  void save(FILE* f, Format format) const;
  void save(const char* filename, Format format) const;
  void save(const std::string& filename, Format format) const;
  std::string save(Format format) const;
  std::string png_data_url() const;

  // read/write functions
  void clear(uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void clear(uint32_t color);
  void read_pixel(ssize_t x, ssize_t y, uint64_t* r, uint64_t* g, uint64_t* b, uint64_t* a = nullptr) const;
  uint32_t read_pixel(ssize_t x, ssize_t y) const;
  void write_pixel(ssize_t x, ssize_t y, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void write_pixel(ssize_t x, ssize_t y, uint32_t color);

  // canvas functions
  void reverse_horizontal();
  void reverse_vertical();
  void set_channel_width(uint8_t new_width);
  void set_has_alpha(bool has_alpha);
  void set_alpha_from_mask_color(uint64_t r, uint64_t g, uint64_t b);
  void set_alpha_from_mask_color(uint32_t c);

  // drawing functions
  // note: no drawing functions respect the alpha channel - they all set it to
  // the given alpha value and ignore whatever's already in the image
  void draw_line(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2, uint64_t r,
      uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void draw_line(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2, uint32_t color);
  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y,
      ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b,
      uint64_t a = 0xFF);
  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y,
      ssize_t dash_length, uint32_t color);
  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2,
      ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b,
      uint64_t a = 0xFF);
  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2,
      ssize_t dash_length, uint32_t color);
  void draw_text_v(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
      uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
      uint64_t bb, uint64_t ba, const char* fmt, va_list va);
  void draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
      uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
      uint64_t bb, uint64_t ba, const char* fmt, ...) ATTR_PRINTF(14, 15);
  void draw_text(ssize_t x, ssize_t y,
      uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg,
      uint64_t bb, uint64_t ba, const char* fmt, ...) ATTR_PRINTF(12, 13);
  void draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height,
      uint32_t color, uint32_t background, const char* fmt, ...)
      ATTR_PRINTF(8, 9);
  void draw_text(ssize_t x, ssize_t y, uint32_t color, uint32_t background,
      const char* fmt, ...) ATTR_PRINTF(6, 7);
  void draw_text(ssize_t x, ssize_t y, uint32_t color, const char* fmt, ...)
      ATTR_PRINTF(5, 6);
  void fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint64_t r,
      uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint32_t color);
  void invert();

  // copy functions
  // this doesn't respect alpha; the written pixels will have the same alpha as
  // in the source image
  void blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, uint32_t transparent_color);
  void mask_blit_dst(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b);
  void mask_blit_dst(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, uint32_t transparent_color);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w,
      ssize_t h, ssize_t sx, ssize_t sy, const Image& mask);
  void custom_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy, std::function<void(uint32_t&, uint32_t)> per_pixel_fn);
  void custom_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy, std::function<void(uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t, uint64_t, uint64_t, uint64_t)> per_pixel_fn);

  void resize_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy, ssize_t sw, ssize_t sh);

  // blend functions
  void blend_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy);
  void blend_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy, uint64_t source_alpha);

private:
  union DataPtrs {
    void* raw;
    uint8_t* as8;
    uint16_t* as16;
    uint32_t* as32;
    uint64_t* as64;
  };

  ssize_t width;
  ssize_t height;
  bool has_alpha;
  uint8_t channel_width;
  uint64_t max_value;
  DataPtrs data;

  void load(FILE* f);

  template <typename Writer>
  void save_helper(Format format, Writer&& writer) const;
};
