#pragma once

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <zlib.h>

#include <format>
#include <functional>
#include <stdexcept>
#include <string>

#include "Encoding.hh"
#include "ImageTextFont.hh"
#include "Platform.hh"
#include "Strings.hh"

namespace phosg {

////////////////////////////////////////////////////////////////////////////////

constexpr uint8_t get_r(uint32_t color) {
  return (color >> 24) & 0xFF;
}
constexpr uint8_t get_g(uint32_t color) {
  return (color >> 16) & 0xFF;
}
constexpr uint8_t get_b(uint32_t color) {
  return (color >> 8) & 0xFF;
}
constexpr uint8_t get_a(uint32_t color) {
  return color & 0xFF;
}
constexpr uint32_t rgba8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF) {
  return (r << 24) | (g << 16) | (b << 8) | a;
}
constexpr uint32_t rgba8888_gray(uint8_t v, uint8_t a = 0xFF) {
  return (v << 24) | (v << 16) | (v << 8) | a;
}

constexpr uint32_t replace_alpha(uint32_t c, uint8_t alpha) {
  return (c & 0xFFFFFF00) | alpha;
}

constexpr uint32_t rgba8888_for_xrgb1555(uint16_t color) {
  return rgba8888(
      ((color >> 7) & 0xF8) | ((color >> 12) & 0x07),
      ((color >> 2) & 0xF8) | ((color >> 7) & 0x07),
      ((color << 3) & 0xF8) | ((color >> 2) & 0x07),
      0xFF);
}
constexpr uint16_t xrgb1555_for_rgba8888(uint32_t color) {
  return ((get_r(color) << 7) & 0x7C00) | ((get_g(color) << 2) & 0x03E0) | ((get_b(color) >> 3) & 0x001F);
}

constexpr uint32_t rgba8888_for_argb1555(uint16_t color) {
  return rgba8888(
      ((color >> 7) & 0xF8) | ((color >> 12) & 0x07),
      ((color >> 2) & 0xF8) | ((color >> 7) & 0x07),
      ((color << 3) & 0xF8) | ((color >> 2) & 0x07),
      (color & 0x8000) ? 0xFF : 0x00);
}
constexpr uint16_t argb1555_for_rgba8888(uint32_t color) {
  return ((get_a(color) << 8) & 0x8000) |
      ((get_r(color) << 7) & 0x7C00) |
      ((get_g(color) << 2) & 0x03E0) |
      ((get_b(color) >> 3) & 0x001F);
}

constexpr uint32_t rgba8888_for_rgb565(uint16_t color) {
  return rgba8888(
      ((color >> 8) & 0xF8) | ((color >> 13) & 0x07),
      ((color >> 3) & 0xFC) | ((color >> 9) & 0x03),
      ((color << 3) & 0xF8) | ((color >> 2) & 0x07),
      0xFF);
}
constexpr uint16_t rgb565_for_rgba8888(uint32_t color) {
  return ((get_r(color) << 8) & 0xF800) | ((get_g(color) << 3) & 0x07E0) | ((get_b(color) >> 3) & 0x001F);
}

constexpr uint32_t rgba8888_for_argb8888(uint32_t color) {
  return ((color << 8) & 0xFFFFFF00) | ((color >> 24) & 0x000000FF);
}
constexpr uint32_t argb8888_for_rgba8888(uint32_t color) {
  return ((color >> 8) & 0x00FFFFFF) | ((color << 24) & 0xFF000000);
}

constexpr uint32_t alpha_blend(uint32_t orig_color, uint32_t new_color) {
  uint8_t a = get_a(new_color);
  return rgba8888(
      (a * get_r(new_color) + (0xFF - a) * get_r(orig_color)) / 0xFF,
      (a * get_g(new_color) + (0xFF - a) * get_g(orig_color)) / 0xFF,
      (a * get_b(new_color) + (0xFF - a) * get_b(orig_color)) / 0xFF,
      (a * get_a(new_color) + (0xFF - a) * get_a(orig_color)) / 0xFF);
}

constexpr uint32_t invert(uint32_t color) {
  return rgba8888(0xFF - get_r(color), 0xFF - get_g(color), 0xFF - get_b(color), get_a(color));
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

enum class PixelFormat {
  G1 = 0, // White and black (0=white, 1=black)
  GA11, // White and black + alpha
  G8, // 8-bit grayscale
  GA88, // Grayscale + alpha
  XRGB1555,
  ARGB1555,
  RGB565,
  RGB888,
  RGBA8888,
  ARGB8888,
};

template <PixelFormat Format>
struct PixelBuffer {
  using DataT = void;
  static constexpr bool HAS_ALPHA = false;

  DataT* data;
  size_t w;
  size_t h;
};

template <>
struct PixelBuffer<PixelFormat::G1> {
  using DataT = uint8_t;
  static constexpr bool HAS_ALPHA = false;

  DataT* data;
  size_t w;
  size_t h;

  static size_t row_bytes(size_t w) {
    return (w + 7) >> 3;
  }
  size_t row_bytes() const {
    return this->row_bytes(this->w);
  }
  static size_t data_size(size_t w, size_t h) {
    return (PixelBuffer<PixelFormat::G1>::row_bytes(w) * h);
  }
  uint32_t read(size_t x, size_t y) const {
    uint8_t block = this->data[y * this->row_bytes() + (x >> 3)];
    return ((block << (x & 7)) & 0x80) ? 0x000000FF : 0xFFFFFFFF;
  }
  void write(size_t x, size_t y, uint32_t color) {
    uint8_t& block = this->data[y * this->row_bytes() + (x >> 3)];
    if (((get_r(color) + get_g(color) + get_b(color)) / 3) >= 0x80) {
      block &= ~(0x80 >> (x & 7)); // Set to white
    } else {
      block |= (0x80 >> (x & 7)); // Set to black
    }
  }
  void write_row(size_t y, const void* data, size_t pixel_count) {
    memcpy(&this->data[y * this->row_bytes()], data, (pixel_count + 7) >> 3);
  }
};

template <>
struct PixelBuffer<PixelFormat::GA11> {
  using DataT = uint8_t;
  static constexpr bool HAS_ALPHA = true;

  DataT* data;
  size_t w;
  size_t h;

  static size_t row_bytes(size_t w) {
    return (w + 3) >> 2;
  }
  size_t row_bytes() const {
    return this->row_bytes(this->w);
  }
  static size_t data_size(size_t w, size_t h) {
    return (PixelBuffer<PixelFormat::GA11>::row_bytes(w) * h);
  }
  uint32_t read(size_t x, size_t y) const {
    uint8_t block = this->data[y * this->row_bytes() + (x >> 2)];
    static const uint32_t values[4] = {0x00000000, 0xFFFFFFFF, 0x00000000, 0x000000FF};
    return values[(block >> (6 - ((x & 3) << 1))) & 3];
  }
  void write(size_t x, size_t y, uint32_t color) {
    uint8_t& block = this->data[y * this->row_bytes() + (x >> 2)];
    if (get_a(color) < 0x80) {
      block &= ~(0xC0 >> ((x & 3) << 1));
    } else if (((get_r(color) + get_g(color) + get_b(color)) / 3) >= 0x80) {
      block = (block & ~(0xC0 >> ((x & 3) << 1))) | (0x40 >> ((x & 3) << 1));
    } else {
      block = (block & ~(0xC0 >> ((x & 3) << 1))) | (0xC0 >> ((x & 3) << 1));
    }
  }
  void write_row(size_t y, const void* data, size_t pixel_count) {
    memcpy(&this->data[y * this->row_bytes()], data, (pixel_count + 3) >> 2);
  }
};

template <>
struct PixelBuffer<PixelFormat::G8> {
  using DataT = uint8_t;
  static constexpr bool HAS_ALPHA = false;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h;
  }
  uint32_t read(size_t x, size_t y) const {
    uint8_t ret = this->data[y * this->w + x];
    return rgba8888(ret, ret, ret, 0xFF);
  }
  void write(size_t x, size_t y, uint32_t color) {
    this->data[y * this->w + x] = (get_r(color) + get_g(color) + get_b(color)) / 3;
  }
};

template <>
struct PixelBuffer<PixelFormat::GA88> {
  using DataT = be_uint16_t;
  static constexpr bool HAS_ALPHA = true;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 2;
  }
  uint32_t read(size_t x, size_t y) const {
    uint16_t ret = this->data[y * this->w + x];
    uint8_t g = (ret >> 8);
    return rgba8888(g, g, g, ret & 0xFF);
  }
  void write(size_t x, size_t y, uint32_t color) {
    uint8_t g = (get_r(color) + get_g(color) + get_b(color)) / 3;
    this->data[y * this->w + x] = (g << 8) | get_a(color);
  }
};

template <>
struct PixelBuffer<PixelFormat::XRGB1555> {
  using DataT = be_uint16_t;
  static constexpr bool HAS_ALPHA = false;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 2;
  }
  uint32_t read(size_t x, size_t y) const {
    return rgba8888_for_xrgb1555(this->data[y * this->w + x]);
  }
  void write(size_t x, size_t y, uint32_t color) {
    this->data[y * this->w + x] = xrgb1555_for_rgba8888(color);
  }
};

template <>
struct PixelBuffer<PixelFormat::ARGB1555> {
  using DataT = be_uint16_t;
  static constexpr bool HAS_ALPHA = true;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 2;
  }
  uint32_t read(size_t x, size_t y) const {
    return rgba8888_for_argb1555(this->data[y * this->w + x]);
  }
  void write(size_t x, size_t y, uint32_t color) {
    this->data[y * this->w + x] = argb1555_for_rgba8888(color);
  }
};

template <>
struct PixelBuffer<PixelFormat::RGB565> {
  using DataT = be_uint16_t;
  static constexpr bool HAS_ALPHA = false;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 2;
  }
  uint32_t read(size_t x, size_t y) const {
    return rgba8888_for_rgb565(this->data[y * this->w + x]);
  }
  void write(size_t x, size_t y, uint32_t color) {
    this->data[y * this->w + x] = rgb565_for_rgba8888(color);
  }
};

template <>
struct PixelBuffer<PixelFormat::RGB888> {
  // We have to use uint8_t because 3 isn't a power of 2, so accesses would be
  // misaligned if we didn't
  using DataT = uint8_t;
  static constexpr bool HAS_ALPHA = false;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 3;
  }
  uint32_t read(size_t x, size_t y) const {
    size_t index = (y * this->w + x) * 3;
    return rgba8888(this->data[index], this->data[index + 1], this->data[index + 2], 0xFF);
  }
  void write(size_t x, size_t y, uint32_t color) {
    size_t index = (y * this->w + x) * 3;
    this->data[index] = get_r(color);
    this->data[index + 1] = get_g(color);
    this->data[index + 2] = get_b(color);
  }
};

template <>
struct PixelBuffer<PixelFormat::RGBA8888> {
  using DataT = be_uint32_t;
  static constexpr bool HAS_ALPHA = true;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 4;
  }
  uint32_t read(size_t x, size_t y) const {
    return this->data[y * this->w + x];
  }
  void write(size_t x, size_t y, uint32_t color) {
    this->data[y * this->w + x] = color;
  }
};

template <>
struct PixelBuffer<PixelFormat::ARGB8888> {
  using DataT = be_uint32_t;
  static constexpr bool HAS_ALPHA = true;

  DataT* data;
  size_t w;
  size_t h;

  static size_t data_size(size_t w, size_t h) {
    return w * h * 4;
  }
  uint32_t read(size_t x, size_t y) const {
    uint32_t color = this->data[y * this->w + x];
    return rgba8888_for_argb8888(color);
  }
  void write(size_t x, size_t y, uint32_t color) {
    this->data[y * this->w + x] = argb8888_for_rgba8888(color);
  }
};

/////////////////////////////////////////////////////////////////////////////

enum class ImageFormat {
  GRAYSCALE_PPM = 0,
  COLOR_PPM,
  WINDOWS_BITMAP,
  PNG,
  PNG_DATA_URL,
};

inline const char* file_extension_for_image_format(ImageFormat format) {
  switch (format) {
    case ImageFormat::GRAYSCALE_PPM:
    case ImageFormat::COLOR_PPM:
      return "ppm";
    case ImageFormat::WINDOWS_BITMAP:
      return "bmp";
    case ImageFormat::PNG:
      return "png";
    case ImageFormat::PNG_DATA_URL:
      throw std::logic_error("PNG_DATA_URL does not have a file extension");
    default:
      throw std::logic_error("invalid image format");
  }
}

template <PixelFormat Format>
class Image : public PixelBuffer<Format> {
public:
  using DataT = PixelBuffer<Format>::DataT;
  static constexpr bool HAS_ALPHA = PixelBuffer<Format>::HAS_ALPHA;

protected:
  std::unique_ptr<DataT[]> owned_data;

  static std::unique_ptr<DataT[]> make_owned_data(size_t w, size_t h) {
    return std::make_unique<DataT[]>(PixelBuffer<Format>::data_size(w, h));
  }

  template <PixelFormat OtherFormat>
  friend class Image;

public:
  /////////////////////////////////////////////////////////////////////////////
  // Constructors

  // Construct an Image filled with the given color
  Image(size_t w = 0, size_t h = 0, uint32_t color = 0x00000000) {
    this->w = w;
    this->h = h;
    this->owned_data = this->make_owned_data(w, h);
    this->data = this->owned_data.get();
    this->clear(color);
  }

  // Construct an Image with raw pixels from an external data buffer. The
  // contents of the buffer are copied into the Image.
  static Image<Format> from_data_copy(void* raw_data, size_t w, size_t h) {
    Image<Format> ret;
    ret.w = w;
    ret.h = h;
    ret.owned_data = ret.make_owned_data(w, h);
    ret.data = ret.owned_data.get();
    memcpy(ret.data, raw_data, ret.get_data_size());
  }
  // Construct an Image with raw pixels from an external data buffer
  // without copying the data. It is the caller's responsibility to free the
  // data buffer and to ensure the Image isn't used after the data buffer
  // is freed.
  static Image<Format> from_data_reference(void* raw_data, size_t w, size_t h) {
    Image<Format> ret;
    ret.w = w;
    ret.h = h;
    ret.data = reinterpret_cast<DataT*>(raw_data);
  }

  // File (PPM/BMP) parsing constructor
  static Image<Format> from_file_data(const void* data, size_t size) {
    StringReader r(data, size);
    uint16_t sig = r.get_u16b(0);

    ImageFormat format;
    bool is_extended_ppm = false;
    if (sig == 0x5035) { // P5
      format = ImageFormat::GRAYSCALE_PPM;
    } else if (sig == 0x5036) { // P6
      format = ImageFormat::COLOR_PPM;
    } else if (sig == 0x5037) { // P7
      format = ImageFormat::COLOR_PPM;
      is_extended_ppm = true;
    } else if (sig == 0x424D) { // BM
      format = ImageFormat::WINDOWS_BITMAP;
    } else {
      throw std::runtime_error(std::format("can\'t load image; type signature is {:04X}", sig));
    }

    Image<Format> ret;
    if (format == ImageFormat::GRAYSCALE_PPM || format == ImageFormat::COLOR_PPM) {
      uint64_t new_max_value = 0;
      bool file_has_alpha = false;

      if (is_extended_ppm) {
        if (r.get_line() != "P7") {
          throw std::runtime_error("invalid extended PPM header");
        }
        for (;;) {
          std::string line = r.get_line();
          strip_trailing_whitespace(line);
          if (line.starts_with("WIDTH ")) {
            ret.w = stoull(line.substr(6));
          } else if (line.starts_with("HEIGHT ")) {
            ret.h = stoull(line.substr(7));
          } else if (line.starts_with("DEPTH ")) {
            // We ignore this and use TUPLTYPE instead
          } else if (line.starts_with("MAXVAL ")) {
            new_max_value = stoull(line.substr(7));
          } else if (line.starts_with("TUPLTYPE ")) {
            std::string tuple_type = line.substr(9);
            if (tuple_type == "GRAYSCALE") {
              format = ImageFormat::GRAYSCALE_PPM;
            } else if (tuple_type == "GRAYSCALE_ALPHA") {
              format = ImageFormat::GRAYSCALE_PPM;
              file_has_alpha = true;
            } else if (tuple_type == "RGB") {
              format = ImageFormat::COLOR_PPM;
            } else if (tuple_type == "RGB_ALPHA") {
              format = ImageFormat::COLOR_PPM;
              file_has_alpha = true;
            } else {
              throw std::runtime_error("unsupported tuple type in extended PPM image");
            }
          } else if (line == "ENDHDR") {
            break;
          } else {
            throw std::runtime_error("unknown header command in extended PPM image");
          }
        }

      } else {
        // According to the docs, the end of the header line is "usually" a
        // newline but can technically be any whitespace character. Here we
        // assume it's always a \n, which will probably fail in rare cases
        std::string header_line = r.get_line();
        auto tokens = split(header_line, ' ');
        if (tokens.size() != 4) {
          throw std::runtime_error(std::format("invalid PPM header line: {}", header_line));
        }
        if (tokens[0] != "P6") {
          throw std::logic_error("incorrect header token for P6 PPM: " + tokens[0]);
        }
        ret.w = stoull(tokens[1], nullptr, 10);
        ret.h = stoull(tokens[2], nullptr, 10);
        new_max_value = stoull(tokens[3], nullptr, 10);
      }

      if (ret.w == 0) {
        throw std::runtime_error("width field in PPM header is zero or missing");
      }
      if (ret.h == 0) {
        throw std::runtime_error("height field in PPM header is zero or missing");
      }
      if (new_max_value != 255) {
        throw std::runtime_error("max value field in PPM header is missing, or contains a value other than 255");
      }

      ret.owned_data = Image::make_owned_data(ret.w, ret.h);
      ret.data = ret.owned_data.get();

      for (size_t y = 0; y < ret.h; y++) {
        for (size_t x = 0; x < ret.w; x++) {
          if (format == ImageFormat::GRAYSCALE_PPM) {
            if (!file_has_alpha) {
              uint8_t v = r.get_u8();
              ret.write(x, y, rgba8888(v, v, v, 0xFF));
            } else {
              uint8_t v = r.get_u8();
              uint8_t a = r.get_u8();
              ret.write(x, y, rgba8888(v, v, v, a));
            }
          } else if (!file_has_alpha) {
            uint8_t r_v = r.get_u8();
            uint8_t g_v = r.get_u8();
            uint8_t b_v = r.get_u8();
            ret.write(x, y, rgba8888(r_v, g_v, b_v, 0xFF));
          } else {
            ret.write(x, y, r.get_u32b());
          }
        }
      }

    } else if (format == ImageFormat::WINDOWS_BITMAP) {
      r.go(0);
      WindowsBitmapHeader header;
      header.file_header = r.get<WindowsBitmapFileHeader>();
      uint32_t info_header_size = r.get_u32l(false);
      if (info_header_size > sizeof(header.info_header)) {
        throw std::runtime_error(std::format("unsupported bitmap header: size is {}, maximum supported size is {}",
            header.info_header.header_size, sizeof(header.info_header)));
      }
      memcpy(&header.info_header, r.getv(info_header_size), info_header_size);

      if ((header.info_header.bit_depth != 24) && (header.info_header.bit_depth != 32)) {
        throw std::runtime_error(std::format(
            "can only load 24-bit or 32-bit bitmaps (this is a {}-bit bitmap)", header.info_header.bit_depth));
      }
      if (header.info_header.num_planes != 1) {
        throw std::runtime_error("can only load 1-plane bitmaps");
      }

      bool reverse_row_order = header.info_header.height < 0;
      ret.w = header.info_header.width;
      ret.h = reverse_row_order ? -header.info_header.height.load() : header.info_header.height.load();
      ret.owned_data = Image::make_owned_data(ret.w, ret.h);
      ret.data = ret.owned_data.get();

      r.go(header.file_header.data_offset);

      if (header.info_header.compression == 0) { // BI_RGB
        if ((header.info_header.bit_depth != 24) && (header.info_header.bit_depth != 32)) {
          throw std::runtime_error("bitmap uses BI_RGB but bit depth is not 24 or 32");
        }

        size_t pixel_bytes = header.info_header.bit_depth / 8;
        size_t row_padding_bytes = (4 - ((ret.w * pixel_bytes) % 4)) % 4;

        for (ssize_t y = static_cast<ssize_t>(ret.h) - 1; y >= 0; y--) {
          ssize_t target_y = reverse_row_order ? (ret.h - y - 1) : y;
          for (size_t x = 0; x < ret.w; x++) {
            uint8_t b_v = r.get_u8();
            uint8_t g_v = r.get_u8();
            uint8_t r_v = r.get_u8();
            ret.write(x, target_y, rgba8888(r_v, g_v, b_v, 0xFF));
          }
          r.skip(row_padding_bytes);
        }

      } else if (header.info_header.compression == 3) { // BI_BITFIELDS
        if (header.info_header.bit_depth != 32) {
          throw std::runtime_error("bitmap uses BI_BITFIELDS but bit depth is not 32");
        }

        // We only support bitmaps where channels are entire bytes
        size_t r_offset, g_offset, b_offset, a_offset;
        static const std::unordered_map<uint32_t, size_t> offset_for_bitmask{
            {0xFF000000, 24}, {0x00FF0000, 16}, {0x0000FF00, 8}, {0x000000FF, 0}};
        try {
          r_offset = offset_for_bitmask.at(header.info_header.bitmask_r);
          g_offset = offset_for_bitmask.at(header.info_header.bitmask_g);
          b_offset = offset_for_bitmask.at(header.info_header.bitmask_b);
          a_offset = offset_for_bitmask.at(header.info_header.bitmask_a);
        } catch (const std::out_of_range&) {
          throw std::runtime_error("channel bit field is not 1-byte mask");
        }

        for (ssize_t y = static_cast<ssize_t>(ret.h) - 1; y >= 0; y--) {
          ssize_t target_y = reverse_row_order ? (ret.h - y - 1) : y;
          for (size_t x = 0; x < ret.w; x++) {
            uint32_t color = r.get_u32l();
            uint8_t r_v = (color >> r_offset) & 0xFF;
            uint8_t g_v = (color >> g_offset) & 0xFF;
            uint8_t b_v = (color >> b_offset) & 0xFF;
            uint8_t a_v = (color >> a_offset) & 0xFF;
            ret.write(x, target_y, rgba8888(r_v, g_v, b_v, a_v));
          }
        }

      } else {
        throw std::runtime_error("can only load uncompressed or bitfield bitmaps");
      }
    }

    return ret;
  }

  static Image<Format> from_file_data(const std::string& data) {
    return Image<Format>::from_file_data(data.data(), data.size());
  }

  // Move constructor (must be same pixel format)
  Image(Image<Format>&& other) {
    this->operator=(std::move(other));
  }
  Image& operator=(Image<Format>&& other) {
    this->data = other.data;
    this->owned_data = std::move(other.owned_data);
    this->w = other.w;
    this->h = other.h;
    other.w = 0;
    other.h = 0;
    other.data = nullptr;
    return *this;
  }

  // The copy constructor is intentionally deleted, since copying an Image
  // is potentially very expensive. Instead, we require the caller to call
  // .copy() (below) to ensure they actually want to make a copy.
  Image(const Image<Format>& other) = delete;
  Image& operator=(const Image<Format>& other) = delete;

  Image<Format> copy() const {
    Image<Format> ret;
    ret.w = this->w;
    ret.h = this->h;
    ret.owned_data = ret.make_owned_data(ret.w, ret.h);
    ret.data = ret.owned_data.get();
    memcpy(ret.data, this->data, this->get_data_size());
    return ret;
  }
  template <PixelFormat NewFormat, typename FnT>
    requires std::is_invocable_r_v<uint32_t, FnT, uint32_t>
  Image<NewFormat> change_pixel_format(FnT&& transform_color) const {
    Image<NewFormat> ret;
    ret.w = this->w;
    ret.h = this->h;
    ret.owned_data = ret.make_owned_data(ret.w, ret.h);
    ret.data = ret.owned_data.get();
    for (size_t y = 0; y < this->h; y++) {
      for (size_t x = 0; x < this->w; x++) {
        ret.write(x, y, transform_color(this->read(x, y)));
      }
    }
    return ret;
  }
  template <PixelFormat NewFormat>
  Image<NewFormat> change_pixel_format() const {
    return this->change_pixel_format<NewFormat>([](uint32_t color) { return color; });
  }
  template <PixelFormat NewFormat = PixelFormat::RGB888>
    requires(Format == PixelFormat::G1)
  Image<NewFormat> convert_monochrome_to_color(uint32_t zero_color = 0xFFFFFFFF, uint32_t one_color = 0x000000FF) const {
    return this->change_pixel_format<NewFormat>([zero_color, one_color](uint32_t color) {
      if (color == 0xFFFFFFFF) {
        return zero_color;
      } else if (color == 0x000000FF) {
        return one_color;
      } else {
        throw std::logic_error("invalid color from G1 Image");
      }
    });
  }
  template <PixelFormat NewFormat = PixelFormat::RGBA8888>
    requires(Format == PixelFormat::GA11)
  Image<NewFormat> convert_monochrome_to_color(uint32_t zero_color = 0xFFFFFFFF, uint32_t one_color = 0x000000FF) const {
    return this->change_pixel_format<NewFormat>([zero_color, one_color](uint32_t color) -> uint32_t {
      if (get_a(color) == 0) {
        return 0x00000000;
      } else if (color == 0xFFFFFFFF) {
        return zero_color;
      } else if (color == 0x000000FF) {
        return one_color;
      } else {
        throw std::logic_error("invalid color from G1 Image");
      }
    });
  }

  /////////////////////////////////////////////////////////////////////////////
  // Serialization

  std::string serialize(ImageFormat format) const {
    StringWriter w;

    switch (format) {
      case ImageFormat::GRAYSCALE_PPM:
        throw std::runtime_error("can\'t save grayscale ppm files");

      case ImageFormat::COLOR_PPM: {
        StringWriter w;
        std::string header;
        if constexpr (Image<Format>::HAS_ALPHA) {
          w.write(std::format("P7\nWIDTH {}\nHEIGHT {}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", this->w, this->h));
          for (size_t y = 0; y < this->h; y++) {
            for (size_t x = 0; x < this->w; x++) {
              w.put_u32b(this->read(x, y));
            }
          }
        } else {
          w.write(std::format("P6 {} {} 255\n", this->w, this->h));
          for (size_t y = 0; y < this->h; y++) {
            for (size_t x = 0; x < this->w; x++) {
              uint32_t color = this->read(x, y);
              w.put_u8(get_r(color));
              w.put_u8(get_g(color));
              w.put_u8(get_b(color));
            }
          }
        }
        return std::move(w.str());
      }

      case ImageFormat::WINDOWS_BITMAP: {
        StringWriter w;
        size_t pixel_bytes = 3 + HAS_ALPHA;
        size_t row_padding_bytes = (4 - ((this->w * pixel_bytes) % 4)) % 4;
        uint8_t row_padding_data[4] = {0, 0, 0, 0};

        WindowsBitmapHeader header;
        size_t header_size = sizeof(WindowsBitmapFileHeader) + (HAS_ALPHA ? sizeof(WindowsBitmapInfoHeader) : WindowsBitmapInfoHeader::SIZE24);

        // Clear header so we don't have initialize fields that are 0
        header = {};
        header.file_header.magic = 0x4D42; // 'BM'
        header.file_header.file_size = header_size + (this->w * this->h * pixel_bytes) + (row_padding_bytes * this->h);
        header.file_header.reserved[0] = 0;
        header.file_header.reserved[1] = 0;
        header.file_header.data_offset = header_size;
        header.info_header.header_size = header_size - sizeof(WindowsBitmapFileHeader);
        header.info_header.width = this->w;
        header.info_header.height = this->h;
        header.info_header.num_planes = 1;
        header.info_header.bit_depth = HAS_ALPHA ? 32 : 24;
        header.info_header.compression = HAS_ALPHA ? 3 : 0; // BI_BITFIELDS or BI_RGB
        header.info_header.image_size = HAS_ALPHA ? (this->w * this->h * 4) : 0; // 0 acceptable for BI_RGB
        header.info_header.x_pixels_per_meter = 0x00000B12;
        header.info_header.y_pixels_per_meter = 0x00000B12;
        header.info_header.num_used_colors = 0;
        header.info_header.num_important_colors = 0;
        if constexpr (HAS_ALPHA) {
          // Write V5 header
          header.info_header.bitmask_r = 0x000000FF;
          header.info_header.bitmask_g = 0x0000FF00;
          header.info_header.bitmask_b = 0x00FF0000;
          header.info_header.bitmask_a = 0xFF000000;
          header.info_header.color_space_type = 0x73524742; // LCS_sRGB / 'sRGB'
          header.info_header.render_intent = 8; // LCS_GM_ABS_COLORIMETRIC
        }

        w.write(&header, header_size);

        if constexpr (HAS_ALPHA) {
          // There's no padding and the bitmasks already specify how to read each
          // pixel; just write each row
          for (ssize_t y = this->h - 1; y >= 0; y--) {
            for (size_t x = 0; x < this->w; x++) {
              w.put_u32b(this->read(x, y));
            }
          }

        } else {
          auto row_data_unique = malloc_unique(this->w * 3);
          uint8_t* row_data = reinterpret_cast<uint8_t*>(row_data_unique.get());
          for (ssize_t y = static_cast<ssize_t>(this->h) - 1; y >= 0; y--) {
            for (size_t x = 0; x < this->w; x++) {
              uint32_t color = this->read(x, y);
              row_data[x * 3] = get_b(color);
              row_data[x * 3 + 1] = get_g(color);
              row_data[x * 3 + 2] = get_r(color);
            }
            w.write(row_data, this->w * 3);
            if (row_padding_bytes) {
              w.write(row_padding_data, row_padding_bytes);
            }
          }
        }
        return std::move(w.str());
      }

      case ImageFormat::PNG: {
        StringWriter w;
        auto write_png_chunk = [&w](const char* type, const void* data, size_t size) {
          w.put_u32b(size);
          w.write(type, 4);
          if (size > 0) {
            w.write(data, size);
          }
          // The checksum includes the chunk type, but not the length
          uint32_t crc = ::crc32(0, reinterpret_cast<const Bytef*>(type), 4);
          if (size > 0) {
            crc = ::crc32(crc, reinterpret_cast<const Bytef*>(data), size);
          }
          w.put_u32b(crc);
        };

        constexpr uint8_t signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}; // '\x89PNG\r\n\x1A\n'
        w.write(signature, sizeof(signature));

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
            this->w,
            this->h,
            8,
            static_cast<uint8_t>(HAS_ALPHA ? 6 : 2),
            0, // default compression
            0, // default filter
            0, // non-interlaced
        };
        write_png_chunk("IHDR", &IHDR, 13);

        // Write gAMA chunk
        const be_uint32_t gAMA = 45455; // 1/2.2
        write_png_chunk("gAMA", &gAMA, sizeof(gAMA));

        // Write IDAT chunk
        StringWriter idat_w;
        for (size_t y = 0; y < this->h; y++) {
          idat_w.put_u8(0); // No filter
          for (size_t x = 0; x < this->w; x++) {
            if constexpr (HAS_ALPHA) {
              idat_w.put_u32b(this->read(x, y));
            } else {
              uint32_t color = this->read(x, y);
              idat_w.put_u8(get_r(color));
              idat_w.put_u8(get_g(color));
              idat_w.put_u8(get_b(color));
            }
          }
        }
        if (idat_w.size() != this->h * (1 + this->w * (HAS_ALPHA ? 4 : 3))) {
          throw std::logic_error("PNG IDAT encoding produced incorrect data size");
        }
        uLongf idat_size = compressBound(idat_w.str().size());
        auto idat_data = malloc_unique(idat_size);
        int compress_result = compress2(
            static_cast<Bytef*>(idat_data.get()),
            &idat_size,
            reinterpret_cast<const Bytef*>(idat_w.str().data()),
            idat_w.str().size(),
            9);
        if (compress_result) {
          throw std::runtime_error(std::format("zlib error compressing png data: {}", compress_result));
        }
        write_png_chunk("IDAT", idat_data.get(), idat_size);
        write_png_chunk("IEND", nullptr, 0);
        return std::move(w.str());
      }

      case ImageFormat::PNG_DATA_URL:
        return "data:image/png;base64," + base64_encode(this->serialize(ImageFormat::PNG));

      default:
        throw std::runtime_error("unknown file format in Image::save()");
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Comparators & basic introspection

  bool check(size_t x, size_t y) {
    return (x < this->w) && (y < this->h);
  }

  bool operator==(const Image<Format>& other) const {
    if ((this->w != other.w) || (this->h != other.h)) {
      return false;
    }
    // NOCOMMIT
    for (size_t y = 0; y < this->h; y++) {
      for (size_t x = 0; x < this->w; x++) {
        if (this->read(x, y) != other.read(x, y)) {
          fwrite_fmt(stderr, "Difference at x={} y={}: {:08X} vs. {:08X}\n", x, y, this->read(x, y), other.read(x, y));
        }
      }
    }
    return !memcmp(this->data, other.data, this->get_data_size());
  }
  bool operator!=(const Image<Format>& other) const {
    return !this->operator==(other);
  }

  bool empty() const {
    return (this->w == 0) && (this->h == 0);
  }
  size_t get_width() const {
    return this->w;
  }
  size_t get_height() const {
    return this->h;
  }
  DataT* get_data() {
    return this->data;
  }
  const DataT* get_data() const {
    return this->data;
  }
  static constexpr size_t get_data_size(size_t w, size_t h) {
    return PixelBuffer<Format>::data_size(w, h);
  }
  size_t get_data_size() const {
    return this->get_data_size(this->w, this->h);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Manipulation functions

  void resize(size_t new_w, size_t new_h) {
    if (new_w != this->w || new_h != this->h) {
      Image new_img(new_w, new_h);
      new_img.copy_from(*this, 0, 0, std::min<size_t>(this->w, new_w), std::min<size_t>(this->h, new_h), 0, 0);
      *this = std::move(new_img);
    }
  }

  // Sets all pixels to the given color, without alpha blending
  void clear(uint32_t color) {
    for (size_t y = 0; y < this->h; y++) {
      for (size_t x = 0; x < this->w; x++) {
        this->write(x, y, color);
      }
    }
  }

  void write_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint32_t color) {
    if (!(color & 0x000000FF)) {
      return;
    }
    this->clamp_rect(x, y, w, h);
    for (ssize_t yy = 0; yy < h; yy++) {
      for (ssize_t xx = 0; xx < w; xx++) {
        this->write(x + xx, y + yy, color);
      }
    }
  }

  void blend_rect(ssize_t x, ssize_t y, ssize_t w, ssize_t h, uint32_t color) {
    if (!(color & 0x000000FF)) {
      return;
    }
    this->clamp_rect(x, y, w, h);
    for (ssize_t yy = 0; yy < h; yy++) {
      for (ssize_t xx = 0; xx < w; xx++) {
        this->write(x + xx, y + yy, alpha_blend(this->read(x + xx, y + yy), color));
      }
    }
  }

  // Sets all pixels in the image whose RGB values match the given color
  // (ignoring alpha) to fully transparent.
  void set_alpha_from_mask_color(uint32_t color) {
    color &= 0xFFFFFF00;
    for (size_t y = 0; y < this->h; y++) {
      for (size_t x = 0; x < this->w; x++) {
        if ((this->read(x, y) & 0xFFFFFF00) == color) {
          this->write(x, y, 0x00000000);
        }
      }
    }
  }

  void invert() {
    for (size_t y = 0; y < this->h; y++) {
      for (size_t x = 0; x < this->w; x++) {
        this->write(x, y, this->invert(this->read(x, y)));
      }
    }
  }

  void reverse_horizontal() {
    for (size_t y = 0; y < this->h; y++) {
      for (size_t x = 0; x < this->w / 2; x++) {
        uint32_t p1 = this->read(x, y);
        uint32_t p2 = this->read(this->w - x - 1, y);
        this->write(this->w - x - 1, y, p1);
        this->write(x, y, p2);
      }
    }
  }

  void reverse_vertical() {
    for (size_t y = 0; y < this->h / 2; y++) {
      for (size_t x = 0; x < this->w; x++) {
        uint32_t p1 = this->read(x, y);
        uint32_t p2 = this->read(x, this->h - y - 1);
        this->write(x, this->h - y - 1, p1);
        this->write(x, y, p2);
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Blitting functions

  template <PixelFormat SourceFormat, typename FnT>
    requires std::is_invocable_r_v<uint32_t, FnT, uint32_t, uint32_t>
  void copy_from_with_custom(
      const Image<SourceFormat>& source,
      ssize_t x,
      ssize_t y,
      ssize_t w,
      ssize_t h,
      ssize_t sx,
      ssize_t sy,
      FnT&& per_pixel_fn) {
    this->clamp_blit_dimensions(source, x, y, w, h, sx, sy);
    for (ssize_t yy = 0; yy < h; yy++) {
      for (ssize_t xx = 0; xx < w; xx++) {
        this->write(x + xx, y + yy, per_pixel_fn(this->read(x + xx, y + yy), source.read(sx + xx, sy + yy)));
      }
    }
  }

  // Copies pixels from another image to this one. No blending is performed.
  template <PixelFormat SourceFormat>
  void copy_from(const Image<SourceFormat>& src, size_t dst_x, size_t dst_y, size_t w, size_t h, size_t src_x, size_t src_y) {
    this->copy_from_with_custom(src, dst_x, dst_y, w, h, src_x, src_y, [](uint32_t, uint32_t src_color) -> uint32_t { return src_color; });
  }

  // Copies pixels from another image to this one, blending with the given
  // alpha value. If the source image has an alpha channel, it is ignored.
  template <PixelFormat SourceFormat>
  void copy_from_with_alpha(
      const Image<SourceFormat>& source,
      ssize_t dst_x,
      ssize_t dst_y,
      ssize_t w,
      ssize_t h,
      ssize_t src_x,
      ssize_t src_y,
      uint8_t alpha) {
    if (alpha == 0) {
      return;
    } else if (alpha == 0xFF) {
      this->copy_from(source, dst_x, dst_y, w, h, src_x, src_y);
    } else {
      this->copy_from_with_custom(source, dst_x, dst_y, w, h, src_x, src_y, [alpha](uint32_t dest_color, uint32_t src_color) -> uint32_t {
        return alpha_blend(dest_color, (src_color & 0xFFFFFF00) | alpha);
      });
    }
  }

  // Copies pixels from another image to this one, blending using the alpha
  // channel from the source image. If the source has no alpha channel, this is
  // equivalent to copy_from.
  template <PixelFormat SourceFormat>
  void copy_from_with_blend(
      const Image<SourceFormat>& source, ssize_t dst_x, ssize_t dst_y, ssize_t w, ssize_t h, ssize_t src_x, ssize_t src_y) {
    this->copy_from_with_custom(source, dst_x, dst_y, w, h, src_x, src_y, [](uint32_t dest_color, uint32_t src_color) -> uint32_t {
      // a = 0 and a = FF are common so we special-case them before doing a
      // bunch of arithmetic operations that aren't necessary in those cases
      uint8_t a = get_a(src_color);
      if (a == 0) {
        return dest_color;
      } else if (a == 0xFF) {
        return src_color;
      } else {
        return alpha_blend(dest_color, src_color);
      }
    });
  }

  // Copies pixels from another image to this one, but does not copy pixels
  // whose color in the source image matches the given color.
  template <PixelFormat SourceFormat>
  void copy_from_with_source_color_mask(
      const Image<SourceFormat>& source,
      ssize_t x,
      ssize_t y,
      ssize_t w,
      ssize_t h,
      ssize_t sx,
      ssize_t sy,
      uint32_t transparent_color) {
    transparent_color &= 0xFFFFFF00;
    this->copy_from_with_custom(source, x, y, w, h, sx, sy, [transparent_color](uint32_t dest_color, uint32_t src_color) -> uint32_t {
      return ((src_color & 0xFFFFFF00) == transparent_color) ? dest_color : src_color;
    });
  }

  // Copies pixels from another image to this one, but does not overwrite
  // pixels whose color in this image matches the given color.
  template <PixelFormat SourceFormat>
  void copy_from_with_dest_color_mask(
      const Image<SourceFormat>& source,
      ssize_t x,
      ssize_t y,
      ssize_t w,
      ssize_t h,
      ssize_t sx,
      ssize_t sy,
      uint32_t transparent_color) {
    transparent_color &= 0xFFFFFF00;
    this->copy_from_with_custom(source, x, y, w, h, sx, sy, [transparent_color](uint32_t dest_color, uint32_t src_color) -> uint32_t {
      return ((dest_color & 0xFFFFFF00) == transparent_color) ? dest_color : src_color;
    });
  }

  // Copies pixels from another image to this one, but only copies pixels for
  // which the corresponding pixel in the mask image is not white (FFFFFF).
  template <PixelFormat SourceFormat, PixelFormat MaskFormat>
  void copy_from_with_mask_image(
      const Image<SourceFormat>& source,
      ssize_t x,
      ssize_t y,
      ssize_t w,
      ssize_t h,
      ssize_t sx,
      ssize_t sy,
      const Image<MaskFormat>& mask) {
    // We don't use copy_from_with_custom here because we would need the pixel
    // coordinates within the per-pixel function, and adding those would make
    // the code ugly

    this->clamp_blit_dimensions(source, x, y, w, h, sx, sy);

    // The mask image must cover the entire area to be blitted, but it does NOT
    // have to cover the entire destination or source image. The mask is indexed
    // in source-space, though, so its upper-left corner must be aligned with
    // the source image's upper-left corner.
    if ((mask.get_width() < static_cast<size_t>(sx + w)) ||
        (mask.get_height() < static_cast<size_t>(sy + h))) {
      throw std::runtime_error("mask is too small to cover copied area");
    }

    for (ssize_t yy = 0; yy < h; yy++) {
      for (ssize_t xx = 0; xx < w; xx++) {
        if ((mask.read(sx + xx, sy + yy) & 0xFFFFFF00) != 0xFFFFFF00) {
          this->write(x + xx, y + yy, source.read(sx + xx, sy + yy));
        }
      }
    }
  }

  // Copies pixels from another image to this one, resizing the image using 2-D
  // nearest-neighbor resampling.
  template <PixelFormat SourceFormat>
  void copy_from_with_resize(
      const Image<SourceFormat>& source,
      ssize_t x, ssize_t y, ssize_t w, ssize_t h,
      ssize_t sx, ssize_t sy, ssize_t sw, ssize_t sh) {
    if (w < 0) {
      w = this->w - x;
    }
    if (h < 0) {
      h = this->h - y;
    }
    if (sw < 0) {
      sw = source.w - sx;
    }
    if (sh < 0) {
      sh = source.h - sy;
    }

    // If the dimensions are the same, there's no need to do any floating-point
    // arithmetic; delegate to the faster method
    if (w == sw && h == sh) {
      this->copy_from(source, x, y, w, h, sx, sy);
      return;
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

        uint32_t s11 = source.read(source_x1, source_y1);
        uint32_t s12 = (source_y2_factor != 0.0) ? source.read(source_x1, source_y2) : 0;
        uint32_t s21 = (source_x2_factor != 0.0) ? source.read(source_x2, source_y1) : 0;
        uint32_t s22 = (source_x2_factor != 0.0 && source_y2_factor != 0.0) ? source.read(source_x2, source_y2) : 0;

        uint8_t dr = get_r(s11) * (source_x1_factor * source_y1_factor) +
            get_r(s12) * (source_x1_factor * source_y2_factor) +
            get_r(s21) * (source_x2_factor * source_y1_factor) +
            get_r(s22) * (source_x2_factor * source_y2_factor);
        uint8_t dg = get_g(s11) * (source_x1_factor * source_y1_factor) +
            get_g(s12) * (source_x1_factor * source_y2_factor) +
            get_g(s21) * (source_x2_factor * source_y1_factor) +
            get_g(s22) * (source_x2_factor * source_y2_factor);
        uint8_t db = get_b(s11) * (source_x1_factor * source_y1_factor) +
            get_b(s12) * (source_x1_factor * source_y2_factor) +
            get_b(s21) * (source_x2_factor * source_y1_factor) +
            get_b(s22) * (source_x2_factor * source_y2_factor);
        uint8_t da = get_a(s11) * (source_x1_factor * source_y1_factor) +
            get_a(s12) * (source_x1_factor * source_y2_factor) +
            get_a(s21) * (source_x2_factor * source_y1_factor) +
            get_a(s22) * (source_x2_factor * source_y2_factor);

        this->write(x + xx, y + yy, rgba8888(dr, dg, db, da));
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Drawing functions

  // Uses the Bresenham algorithm to draw a line between the specified points.
  // The endpoints can be outside of the image.
  void draw_line(ssize_t x0, ssize_t y0, ssize_t x1, ssize_t y1, uint32_t color) {
    // If the line is too steep, we step along y rather than x
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
      ssize_t t = x0;
      x0 = y0;
      y0 = t;
      t = x1;
      x1 = y1;
      y1 = t;
    }

    // If the line is backward (x0 is below or to the right of x1), then switch
    // the points
    if (x0 > x1) {
      ssize_t t = x1;
      x1 = x0;
      x0 = t;
      t = y1;
      y1 = y0;
      y0 = t;
    }

    ssize_t dx = x1 - x0;
    ssize_t dy = abs(y1 - y0);
    double error = 0;
    double derror = (double)dy / (double)dx;
    ssize_t ystep = (y0 < y1) ? 1 : -1;
    ssize_t y = y0;
    for (ssize_t x = x0; x <= x1; x++) {
      if (steep) {
        if (this->check(y, x)) {
          this->write(y, x, color);
        }
      } else {
        if (this->check(x, y)) {
          this->write(x, y, color);
        }
      }
      error += derror;

      // If we have passed the center of this row, move to the next row
      if (error >= 0.5) {
        y += ystep;
        error -= 1.0;
      }
    }
  }

  void draw_horizontal_line(ssize_t x1, ssize_t x2, ssize_t y, ssize_t dash_length, uint32_t color) {
    for (ssize_t x = x1; x <= x2; x++) {
      if ((!dash_length || !((x / dash_length) & 1)) && this->check(x, y)) {
        this->write(x, y, color);
      }
    }
  }

  void draw_vertical_line(ssize_t x, ssize_t y1, ssize_t y2, ssize_t dash_length, uint32_t color) {
    for (ssize_t y = y1; y <= y2; y++) {
      if ((!dash_length || !((y / dash_length) & 1)) && this->check(x, y)) {
        this->write(x, y, color);
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // Text functions. These are generally for debugging and reverse-engineering;
  // they use a fixed-size ASCII-only font.

  template <typename... ArgTs>
  void draw_text(
      ssize_t x,
      ssize_t y,
      ssize_t* width,
      ssize_t* height,
      uint32_t text_color,
      uint32_t bg_color,
      std::format_string<ArgTs...> fmt,
      ArgTs&&... args) {

    std::string text = std::format(std::forward<std::format_string<ArgTs...>>(fmt), std::forward<ArgTs>(args)...);

    ssize_t max_x_pos = 0;
    ssize_t x_pos = x, y_pos = y;
    for (size_t z = 0; z < text.size(); z++) {
      uint8_t ch = text[z];
      if (ch == '\r') {
        continue;
      }
      if (ch == '\n') {
        this->blend_rect(x_pos - 1, y_pos - 1, 1, 9, bg_color);
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

      this->blend_rect(x_pos - 1, y_pos - 1, 6, 9, bg_color);
      for (ssize_t yy = 0; yy < 7; yy++) {
        for (ssize_t xx = 0; xx < 5; xx++) {
          if (font[ch][yy * 5 + xx] && this->check(x_pos + xx, y_pos + yy)) {
            this->write(x_pos + xx, y_pos + yy, text_color);
          }
        }
      }

      x_pos += 6;
    }

    this->blend_rect(x_pos - 1, y_pos - 1, 1, 9, bg_color);

    if (width) {
      *width = (x_pos > max_x_pos ? x_pos : max_x_pos) - x;
    }
    if (height) {
      *height = y_pos + 7 - y;
    }
  }

  template <typename... ArgTs>
  void draw_text(
      ssize_t x, ssize_t y, uint32_t text_color, uint32_t bg_color, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text(
        x, y, nullptr, nullptr, text_color, bg_color,
        std::forward<std::format_string<ArgTs...>>(fmt), std::forward<ArgTs>(args)...);
  }
  template <typename... ArgTs>
  void draw_text(ssize_t x, ssize_t y, uint32_t text_color, std::format_string<ArgTs...> fmt, ArgTs&&... args) {
    this->draw_text(
        x, y, nullptr, nullptr, text_color, 0x00000000,
        std::forward<std::format_string<ArgTs...>>(fmt), std::forward<ArgTs>(args)...);
  }

protected:
  void clamp_rect(ssize_t& x, ssize_t& y, ssize_t& w, ssize_t& h) {
    // Resize the rect if it's out of bounds
    if (x < 0) {
      w += x;
      x = 0;
    }
    if (y < 0) {
      h += y;
      y = 0;
    }
    if (x + w > static_cast<ssize_t>(this->w)) {
      w = this->w - x;
    }
    if (y + h > static_cast<ssize_t>(this->h)) {
      h = this->h - y;
    }
  }

  template <PixelFormat SourceFormat>
  void clamp_blit_dimensions(
      const Image<SourceFormat>& source,
      ssize_t& dst_x,
      ssize_t& dst_y,
      ssize_t& w,
      ssize_t& h,
      ssize_t& src_x,
      ssize_t& src_y) {
    // If the width or height are negative, use the entire source image
    if (w < 0) {
      w = source.w;
    }
    if (h < 0) {
      h = source.h;
    }
    // If the source coordinates are negative, trim off the left/top
    if (src_x < 0) {
      dst_x -= src_x;
      w += src_x;
      src_x = 0;
    }
    if (src_y < 0) {
      dst_y -= src_y;
      h += src_y;
      src_y = 0;
    }
    // If the dest coordinates are negative, trim off the left/top
    if (dst_x < 0) {
      src_x -= dst_x;
      w += dst_x;
      dst_x = 0;
    }
    if (dst_y < 0) {
      src_y -= dst_y;
      h += dst_y;
      dst_y = 0;
    }
    // If the area extends beyond the source, trim off the right/bottom
    if (src_x + w > static_cast<ssize_t>(source.get_width())) {
      w = source.get_width() - src_x;
    }
    if (src_y + h > static_cast<ssize_t>(source.get_height())) {
      h = source.get_height() - src_y;
    }
    // If the area extends beyond the dest, trim off the right/bottom
    if (dst_x + w > static_cast<ssize_t>(this->w)) {
      w = this->w - dst_x;
    }
    if (dst_y + h > static_cast<ssize_t>(this->h)) {
      h = this->h - dst_y;
    }

    // If either width or height are negative at this point, then the entire
    // area is out of bounds for either the source or dest; make the area empty
    if (w < 0 || h < 0) {
      w = 0;
      h = 0;
    }
  }
};

using ImageG1 = Image<PixelFormat::G1>;
using ImageGA11 = Image<PixelFormat::GA11>;
using ImageG8 = Image<PixelFormat::G8>;
using ImageGA88 = Image<PixelFormat::GA88>;
using ImageXRGB1555 = Image<PixelFormat::XRGB1555>;
using ImageARGB1555 = Image<PixelFormat::ARGB1555>;
using ImageRGB565 = Image<PixelFormat::RGB565>;
using ImageRGB888 = Image<PixelFormat::RGB888>;
using ImageRGBA8888 = Image<PixelFormat::RGBA8888>;
using ImageARGB8888 = Image<PixelFormat::ARGB8888>;

} // namespace phosg
