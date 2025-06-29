#pragma once

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <format>
#include <functional>
#include <stdexcept>
#include <string>

#include "Platform.hh"

namespace phosg {

// An Image represents a drawing canvas. This class is fairly simple; it
// supports reading/writing individual pixels, drawing lines, and saving the
// image as a PPM or Windows BMP file.
class Image {
public:
  Image();

  // Construct an Image with an external data buffer. If copy_data is false, it
  // is the caller's responsibility to free the data buffer and to ensure the
  // Image isn't used after the data buffer is freed.
  Image(void* raw_data, size_t w, size_t h, bool has_alpha = false, bool copy_data = true);

  // Construct a black-filled Image
  Image(size_t x, size_t y, bool has_alpha = false);

  // Load a file (autodetect format)
  explicit Image(FILE* f);
  explicit Image(const char* filename);
  explicit Image(const std::string& filename);

  // Load from a file (raw data in RGB888 or RGBA8888 format)
  Image(FILE* f, ssize_t width, ssize_t height, bool has_alpha = false);
  Image(const char* filename, ssize_t width, ssize_t height, bool has_alpha = false);
  Image(const std::string& filename, ssize_t width, ssize_t height, bool has_alpha = false);

  Image(const Image&);
  Image(Image&&);
  const Image& operator=(const Image& other);
  Image& operator=(Image&& other);

  ~Image();

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
  inline void* get_data() {
    return this->data.raw;
  }
  inline const void* get_data() const {
    return this->data.raw;
  }
  inline size_t get_data_size() const {
    return this->width * this->height * (3 + this->has_alpha);
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

  // Read/write functions
  void clear(uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void clear(uint32_t color);
  void read_pixel(ssize_t x, ssize_t y, uint64_t* r, uint64_t* g, uint64_t* b, uint64_t* a = nullptr) const;
  uint32_t read_pixel(ssize_t x, ssize_t y) const;
  void write_pixel(ssize_t x, ssize_t y, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void write_pixel(ssize_t x, ssize_t y, uint32_t color);

  // Canvas functions
  void reverse_horizontal();
  void reverse_vertical();
  inline void set_dimensions(ssize_t new_w, ssize_t new_h) {
    this->set_canvas_properties(new_w, new_h, this->has_alpha);
  }
  inline void set_has_alpha(bool new_has_alpha) {
    this->set_canvas_properties(this->width, this->height, new_has_alpha);
  }
  void set_canvas_properties(ssize_t new_w, ssize_t new_h, bool new_has_alpha);
  void set_alpha_from_mask_color(uint64_t r, uint64_t g, uint64_t b);
  void set_alpha_from_mask_color(uint32_t c);

  // Drawing functions
  // Note: no drawing functions respect the alpha channel - they all set it to
  // the given alpha value and ignore whatever's already in the image
  void draw_line(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void draw_line(ssize_t x1, ssize_t y1, ssize_t x2, ssize_t y2, uint32_t color);
  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y, ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y, ssize_t dash_length, uint32_t color);
  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2, ssize_t dash_length, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2, ssize_t dash_length, uint32_t color);
  void fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint64_t r, uint64_t g, uint64_t b, uint64_t a = 0xFF);
  void fill_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint32_t color);
  void invert();

  // Text functions. These are generally for debugging and reverse-engineering;
  // they use a fixed-size ASCII-only font.
  // TODO: This is super ugly. Make it less ugly.
  void draw_text_string(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height, uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg, uint64_t bb, uint64_t ba, const std::string& text);
  inline void draw_text_string(ssize_t x, ssize_t y, uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg, uint64_t bb, uint64_t ba, const std::string& text) {
    this->draw_text_string(x, y, nullptr, nullptr, r, g, b, a, br, bg, bb, ba, text);
  }
  inline void draw_text_string(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height, uint32_t color, uint32_t background, const std::string& text) {
    this->draw_text_string(x, y, width, height, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (background >> 24) & 0xFF, (background >> 16) & 0xFF, (background >> 8) & 0xFF, background & 0xFF, text);
  }
  inline void draw_text_string(ssize_t x, ssize_t y, uint32_t color, uint32_t background, const std::string& text) {
    this->draw_text_string(x, y, nullptr, nullptr, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (background >> 24) & 0xFF, (background >> 16) & 0xFF, (background >> 8) & 0xFF, background & 0xFF, text);
  }
  inline void draw_text_string(ssize_t x, ssize_t y, uint32_t color, const std::string& text) {
    this->draw_text_string(x, y, nullptr, nullptr, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0, 0, 0, 0, text);
  }

  template <typename... ArgTs>
  void draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height, uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg, uint64_t bb, uint64_t ba, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text_string(x, y, width, height, r, g, b, a, br, bg, bb, ba, std::format(fmt, std::forward<ArgTs>(args)...));
  }
  template <typename... ArgTs>
  void draw_text(ssize_t x, ssize_t y, uint64_t r, uint64_t g, uint64_t b, uint64_t a, uint64_t br, uint64_t bg, uint64_t bb, uint64_t ba, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text_string(x, y, nullptr, nullptr, r, g, b, a, br, bg, bb, ba, std::format(fmt, std::forward<ArgTs>(args)...));
  }
  template <typename... ArgTs>
  void draw_text(ssize_t x, ssize_t y, ssize_t* width, ssize_t* height, uint32_t color, uint32_t background, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text_string(x, y, width, height, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (background >> 24) & 0xFF, (background >> 16) & 0xFF, (background >> 8) & 0xFF, background & 0xFF, std::format(fmt, std::forward<ArgTs>(args)...));
  }
  template <typename... ArgTs>
  void draw_text(ssize_t x, ssize_t y, uint32_t color, uint32_t background, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text_string(x, y, nullptr, nullptr, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (background >> 24) & 0xFF, (background >> 16) & 0xFF, (background >> 8) & 0xFF, background & 0xFF, std::format(fmt, std::forward<ArgTs>(args)...));
  }
  template <typename... ArgTs>
  void draw_text(ssize_t x, ssize_t y, uint32_t color, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text_string(x, y, nullptr, nullptr, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0, 0, 0, 0, std::format(fmt, std::forward<ArgTs>(args)...));
  }

  // Copy functions. These don't respect alpha; the written pixels will have
  // the same alpha as in the source image
  void blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, uint32_t transparent_color);
  void mask_blit_dst(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, uint64_t r, uint64_t g, uint64_t b);
  void mask_blit_dst(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, uint32_t transparent_color);
  void mask_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, const Image& mask);
  void custom_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, std::function<void(uint32_t&, uint32_t)> per_pixel_fn);
  void custom_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, std::function<void(uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t, uint64_t, uint64_t, uint64_t)> per_pixel_fn);

  void resize_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, ssize_t sw, ssize_t sh);

  // Blend functions
  void blend_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy);
  void blend_blit(const Image& source, ssize_t x, ssize_t y, ssize_t w, ssize_t h, ssize_t sx, ssize_t sy, uint64_t source_alpha);

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
  DataPtrs data;
  bool data_owned;

  void load(FILE* f);

  template <typename Writer>
  void save_helper(Format format, Writer&& writer) const;
};

// A BitmapImage represents a monochrome (black and white) drawing canvas
class BitmapImage {
public:
  BitmapImage();
  BitmapImage(size_t w, size_t h);

  BitmapImage(FILE* f, size_t width, size_t height);
  BitmapImage(const char* filename, size_t width, size_t height);
  BitmapImage(const std::string& filename, size_t width, size_t height);

  BitmapImage(const BitmapImage&);
  BitmapImage(BitmapImage&&);
  const BitmapImage& operator=(const BitmapImage& other);
  BitmapImage& operator=(BitmapImage&& other);

  ~BitmapImage();

  bool operator==(const BitmapImage& other) const;
  bool operator!=(const BitmapImage& other) const;

  inline size_t get_width() const {
    return this->width;
  }
  inline size_t get_height() const {
    return this->height;
  }
  inline void* get_data() {
    return this->data;
  }
  inline const void* get_data() const {
    return this->data;
  }
  inline size_t get_data_size() const {
    return this->height * this->row_bytes;
  }

  inline bool empty() const {
    return (this->data == nullptr);
  }

  void clear(bool v);
  bool read_pixel(size_t x, size_t y) const;
  void write_pixel(size_t x, size_t y, bool v);
  void write_row(size_t y, const void* in_data, size_t size);

  void invert();

  Image to_color(uint32_t false_color, uint32_t true_color, bool add_alpha) const;

private:
  size_t width;
  size_t height;
  size_t row_bytes;
  uint8_t* data;
};

} // namespace phosg
