#define _STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <sys/time.h>

#include <filesystem>
#include <format>
#include <vector>

#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;
using namespace phosg;

static const vector<uint32_t> colors{
    0xFF0000C0, 0xFF8000C0, 0xFFFF00C0, 0x00FF00C0, 0x00FFFFC0, 0x0000FFC0, 0xFF00FFC0, 0xFFFFFFC0};

template <PixelFormat Format>
void test_pixel_format(const char* format_name) {
  Image<Format> img(180, 190);

  {
    fwrite_fmt(stderr, "-- [Image:{}] metadata\n", format_name);
    expect_eq(180, img.get_width());
    expect_eq(190, img.get_height());
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] clear\n", format_name);
    img.clear(0x20202020);
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] axis-aligned lines\n", format_name);
    for (size_t x = 0; x < 8; x++) {
      uint32_t c = colors[x];
      img.draw_horizontal_line(5, 175, 90 + x, x, c);
      img.draw_vertical_line(5 + x, 100, 185, x, c);
    }
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] rects\n", format_name);
    img.blend_rect(3, 98, 48, 32, 0xFFFFFF80);
    img.write_rect(3, 130, 48, 32, 0xFFFFFF80);
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] non-axis-aligned lines\n", format_name);
    const vector<pair<ssize_t, ssize_t>> points({
        pair<ssize_t, ssize_t>(0, 0),
        pair<ssize_t, ssize_t>(0, 20),
        pair<ssize_t, ssize_t>(0, 40),
        pair<ssize_t, ssize_t>(0, 60),
        pair<ssize_t, ssize_t>(0, 80),
        pair<ssize_t, ssize_t>(20, 80),
        pair<ssize_t, ssize_t>(40, 80),
        pair<ssize_t, ssize_t>(60, 80),
        pair<ssize_t, ssize_t>(80, 80),
        pair<ssize_t, ssize_t>(80, 60),
        pair<ssize_t, ssize_t>(80, 40),
        pair<ssize_t, ssize_t>(80, 20),
        pair<ssize_t, ssize_t>(80, 0),
        pair<ssize_t, ssize_t>(60, 0),
        pair<ssize_t, ssize_t>(40, 0),
        pair<ssize_t, ssize_t>(20, 0),
    });
    for (size_t x = 0; x < 8; x++) {
      uint32_t c = colors[x];
      img.draw_line(points[x].first + 5, points[x].second + 5, points[x + 8].first + 5, points[x + 8].second + 5, c);
      img.draw_line(points[x + 8].first + 90, points[x + 8].second + 5, points[x].first + 90, points[x].second + 5, c);
    }
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] copy_from_blend\n", format_name);
    img.copy_from_with_blend(img, 40, 105, 80, 80, 5, 5);
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] copy_from_with_source_color_mask\n", format_name);
    img.copy_from_with_source_color_mask(img, 80, 105, 80, 80, 5, 5, 0x20202000);
  }

  {
    fwrite_fmt(stderr, "-- [Image:{}] copy + equality\n", format_name);
    Image<Format> img2 = img.copy();
    expect_eq(img, img2);
  }

  array<ImageFormat, 3> formats{ImageFormat::COLOR_PPM, ImageFormat::WINDOWS_BITMAP, ImageFormat::PNG};
  for (auto format : formats) {
    const char* ext = file_extension_for_image_format(format);

    fwrite_fmt(stderr, "-- [Image:{}/{}] serialize\n", format_name, ext);
    string serialized = img.serialize(format);
    if (format != ImageFormat::PNG) {
      fwrite_fmt(stderr, "-- [Image:{}/{}] parse\n", format_name, ext);
      expect_eq(Image<Format>::from_file_data(serialized), img);
    }

    string reference_filename = std::format("reference/ImageTestReference.{}.{}", format_name, ext);
    fwrite_fmt(stderr, "-- [Image:{}/{}] vs. reference\n", format_name, ext);
    if (std::filesystem::is_regular_file(reference_filename)) {
      expect_eq(load_file(reference_filename), serialized);
    } else {
      fwrite_fmt(stderr, "warning: reference file {} not found; skipping verification\n", reference_filename);
      save_file(std::format("ImageTestResult.{}.{}", format_name, ext), serialized);
    }

    if constexpr (Format == PixelFormat::G1) {
      fwrite_fmt(stderr, "-- [Image:{}/{}] colorize\n", format_name, ext);
      ImageRGBA8888 color_img = img.template convert_monochrome_to_color<PixelFormat::RGBA8888>(0xFF00FFFF, 0x0000FFFF);

      fwrite_fmt(stderr, "-- [Image:{}/{}] colorized serialize\n", format_name, ext);
      string color_serialized = color_img.serialize(format);
      if (format != ImageFormat::PNG) {
        fwrite_fmt(stderr, "-- [Image:{}/{}] colorized parse\n", format_name, ext);
        expect_eq(Image<PixelFormat::RGBA8888>::from_file_data(color_serialized), color_img);
      }

      string color_reference_filename = std::format("reference/ImageTestReference.{}.colorized.{}", format_name, ext);
      fwrite_fmt(stderr, "-- [Image:{}/{}] vs. reference\n", format_name, ext);
      if (std::filesystem::is_regular_file(color_reference_filename)) {
        expect_eq(load_file(color_reference_filename), color_serialized);
      } else {
        fwrite_fmt(stderr, "warning: reference file {} not found; skipping verification\n", color_reference_filename);
      }
    }
  }
}

int main(int, char**) {
  test_pixel_format<PixelFormat::G1>("g1");
  test_pixel_format<PixelFormat::GA11>("ga11");
  test_pixel_format<PixelFormat::G8>("g8");
  test_pixel_format<PixelFormat::GA88>("ga88");
  test_pixel_format<PixelFormat::XRGB1555>("xrgb1555");
  test_pixel_format<PixelFormat::ARGB1555>("argb1555");
  test_pixel_format<PixelFormat::RGB565>("rgb565");
  test_pixel_format<PixelFormat::RGB888>("rgb888");
  test_pixel_format<PixelFormat::RGBA8888>("rgba8888");
  test_pixel_format<PixelFormat::ARGB8888>("argb8888");
  fwrite_fmt(stdout, "ImageTest: all tests passed\n");
  return 0;
}
