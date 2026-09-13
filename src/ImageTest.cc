#define _STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <sys/time.h>

#include <filesystem>
#include <format>
#include <vector>

#include "Arguments.hh"
#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"
#include "UnitTest.hh"

static const std::vector<uint32_t> colors{
    0xFF0000C0, 0xFF8000C0, 0xFFFF00C0, 0x00FF00C0, 0x00FFFFC0, 0x0000FFC0, 0xFF00FFC0, 0xFFFFFFC0};

template <phosg::PixelFormat Format>
void test_pixel_format(const char* format_name, bool save_refs) {
  phosg::Image<Format> img(180, 190);

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] metadata\n", format_name);
    expect_eq(180, img.get_width());
    expect_eq(190, img.get_height());
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] clear\n", format_name);
    img.clear(0xCC00CCFF);
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] flood fill (entire image)\n", format_name);
    img.flood_fill(1, 1, 0x20202020);
    img.flood_fill(1, 1, 0x20202020); // Should behave correctly if the image is already that color
    img.flood_fill(1, 1, img.read(1, 1)); // Should behave correctly if the image is already that color (even if resampled)
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] axis-aligned lines\n", format_name);
    for (size_t x = 0; x < 8; x++) {
      uint32_t c = colors[x];
      img.draw_horizontal_line(5, 175, 90 + x, x, c);
      img.draw_vertical_line(5 + x, 100, 185, x, c);
    }
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] rects\n", format_name);
    img.blend_rect(3, 98, 48, 32, 0xFFFFFF80);
    img.write_rect(3, 130, 48, 32, 0xFFFFFF80);
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] non-axis-aligned lines\n", format_name);
    const std::vector<std::pair<ssize_t, ssize_t>> points{
        std::pair<ssize_t, ssize_t>(0, 0),
        std::pair<ssize_t, ssize_t>(0, 20),
        std::pair<ssize_t, ssize_t>(0, 40),
        std::pair<ssize_t, ssize_t>(0, 60),
        std::pair<ssize_t, ssize_t>(0, 80),
        std::pair<ssize_t, ssize_t>(20, 80),
        std::pair<ssize_t, ssize_t>(40, 80),
        std::pair<ssize_t, ssize_t>(60, 80),
        std::pair<ssize_t, ssize_t>(80, 80),
        std::pair<ssize_t, ssize_t>(80, 60),
        std::pair<ssize_t, ssize_t>(80, 40),
        std::pair<ssize_t, ssize_t>(80, 20),
        std::pair<ssize_t, ssize_t>(80, 0),
        std::pair<ssize_t, ssize_t>(60, 0),
        std::pair<ssize_t, ssize_t>(40, 0),
        std::pair<ssize_t, ssize_t>(20, 0)};
    for (size_t x = 0; x < 8; x++) {
      uint32_t c = colors[x];
      img.draw_line(points[x].first + 5, points[x].second + 5, points[x + 8].first + 5, points[x + 8].second + 5, c);
      img.draw_line(points[x + 8].first + 90, points[x + 8].second + 5, points[x].first + 90, points[x].second + 5, c);
    }
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] copy_from_blend\n", format_name);
    img.copy_from_with_blend(img, 40, 105, 80, 80, 5, 5);
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] copy_from_with_source_color_mask\n", format_name);
    img.copy_from_with_source_color_mask(img, 80, 105, 80, 80, 5, 5, 0x20202000);
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] flood fill (small region)\n", format_name);
    img.flood_fill(27, 117, 0xFF800080);
  }

  {
    phosg::fwrite_fmt(stderr, "-- [Image:{}] copy + equality\n", format_name);
    auto img2 = img.copy();
    expect_eq(img, img2);
  }

  std::array<phosg::ImageFormat, 3> formats{
      phosg::ImageFormat::COLOR_PPM, phosg::ImageFormat::WINDOWS_BITMAP, phosg::ImageFormat::PNG};
  for (auto format : formats) {
    const char* ext = file_extension_for_image_format(format);

    phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] serialize\n", format_name, ext);
    std::string serialized = img.serialize(format);
    if (format != phosg::ImageFormat::PNG) {
      phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] parse\n", format_name, ext);
      expect_eq(phosg::Image<Format>::from_file_data(serialized), img);
    }

    if (save_refs) {
      std::string reference_filename = std::format("reference/ImageTestReference.{}.new.{}", format_name, ext);
      phosg::save_file(reference_filename, serialized);
      phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] ... {}\n", format_name, ext, reference_filename);
    } else {
      std::string reference_filename = std::format("reference/ImageTestReference.{}.{}", format_name, ext);
      phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] vs. reference\n", format_name, ext);
      if (std::filesystem::is_regular_file(reference_filename)) {
        expect_eq(phosg::load_file(reference_filename), serialized);
      } else {
        phosg::fwrite_fmt(stderr, "warning: reference file {} not found; skipping verification\n", reference_filename);
        phosg::save_file(std::format("ImageTestResult.{}.{}", format_name, ext), serialized);
      }
    }

    if constexpr (Format == phosg::PixelFormat::G1) {
      phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] colorize\n", format_name, ext);
      phosg::ImageRGBA8888N color_img = img.template convert_monochrome_to_color<phosg::PixelFormat::RGBA8888_NATIVE>(
          0xFF00FFFF, 0x0000FFFF);

      phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] colorized serialize\n", format_name, ext);
      std::string color_serialized = color_img.serialize(format);
      if (format != phosg::ImageFormat::PNG) {
        phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] colorized parse\n", format_name, ext);
        expect_eq(phosg::Image<phosg::PixelFormat::RGBA8888_NATIVE>::from_file_data(color_serialized), color_img);
      }

      if (save_refs) {
        std::string color_reference_filename = std::format("reference/ImageTestReference.{}.colorized.new.{}", format_name, ext);
        phosg::save_file(color_reference_filename, color_serialized);
        phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] ... {}\n", format_name, ext, color_reference_filename);
      } else {
        std::string color_reference_filename = std::format("reference/ImageTestReference.{}.colorized.{}", format_name, ext);
        phosg::fwrite_fmt(stderr, "-- [Image:{}/{}] colorized vs. reference\n", format_name, ext);
        if (std::filesystem::is_regular_file(color_reference_filename)) {
          expect_eq(phosg::load_file(color_reference_filename), color_serialized);
        } else {
          phosg::fwrite_fmt(stderr, "warning: reference file {} not found; skipping verification\n", color_reference_filename);
        }
      }
    }
  }
}

int main(int argc, char** argv) {
  phosg::Arguments args(argv + 1, argc - 1);
  bool save_refs = args.get<bool>("save-refs");
  args.assert_none_unused();
  test_pixel_format<phosg::PixelFormat::G1>("g1", save_refs);
  test_pixel_format<phosg::PixelFormat::GA11>("ga11", save_refs);
  test_pixel_format<phosg::PixelFormat::G8>("g8", save_refs);
  test_pixel_format<phosg::PixelFormat::GA88_NATIVE>("ga88", save_refs);
  test_pixel_format<phosg::PixelFormat::XRGB1555_NATIVE>("xrgb1555", save_refs);
  test_pixel_format<phosg::PixelFormat::ARGB1555_NATIVE>("argb1555", save_refs);
  test_pixel_format<phosg::PixelFormat::RGB565_NATIVE>("rgb565", save_refs);
  test_pixel_format<phosg::PixelFormat::RGB888>("rgb888", save_refs);
  test_pixel_format<phosg::PixelFormat::RGBA8888_NATIVE>("rgba8888", save_refs);
  test_pixel_format<phosg::PixelFormat::ARGB8888_NATIVE>("argb8888", save_refs);
  phosg::fwrite_fmt(stdout, "ImageTest: all tests passed\n");
  return 0;
}
