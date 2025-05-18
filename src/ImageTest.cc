#define _STDC_FORMAT_MACROS
#include <assert.h>
#include <inttypes.h>
#include <sys/time.h>

#include <format>
#include <vector>

#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;
using namespace phosg;

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r),
        g(g),
        b(b),
        a(a) {}
};

static const vector<Color> colors({
    Color(0xFF, 0x00, 0x00, 0xC0),
    Color(0xFF, 0x80, 0x00, 0xC0),
    Color(0xFF, 0xFF, 0x00, 0xC0),
    Color(0x00, 0xFF, 0x00, 0xC0),
    Color(0x00, 0xFF, 0xFF, 0xC0),
    Color(0x00, 0x00, 0xFF, 0xC0),
    Color(0xFF, 0x00, 0xFF, 0xC0),
    Color(0xFF, 0xFF, 0xFF, 0xC0),
});

int main(int, char**) {

  for (uint8_t has_alpha = 0; has_alpha < 2; has_alpha++) {
    for (uint8_t channel_width = 8; channel_width <= 64; channel_width <<= 1) {
      const char* has_alpha_tag = has_alpha ? "alpha" : "no-alpha";
      Image img(180, 190, has_alpha, channel_width);

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] metadata\n", channel_width, has_alpha_tag);
        expect_eq(180, img.get_width());
        expect_eq(190, img.get_height());
        expect_eq(has_alpha, img.get_has_alpha());
        expect_eq(channel_width, img.get_channel_width());
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] clear\n", channel_width, has_alpha_tag);
        img.clear(0x20, 0x20, 0x20, 0x20);
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] axis-aligned lines\n", channel_width, has_alpha_tag);
        for (size_t x = 0; x < 8; x++) {
          const auto& c = colors[x];
          img.draw_horizontal_line(5, 175, 90 + x, x, c.r, c.g, c.b, c.a);
          img.draw_vertical_line(5 + x, 100, 185, x, c.r, c.g, c.b, c.a);
        }
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] rectangles\n", channel_width, has_alpha_tag);
        img.fill_rect(3, 98, 64, 64, 0xFF, 0xFF, 0xFF, 0x80);
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] non-axis-aligned lines\n", channel_width, has_alpha_tag);
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
          const auto& c = colors[x];
          img.draw_line(points[x].first + 5, points[x].second + 5,
              points[x + 8].first + 5, points[x + 8].second + 5, c.r, c.g, c.b, c.a);
          img.draw_line(points[x + 8].first + 90, points[x + 8].second + 5,
              points[x].first + 90, points[x].second + 5, c.r, c.g, c.b, c.a);
        }
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] blit\n", channel_width, has_alpha_tag);
        img.blit(img, 40, 105, 80, 80, 5, 5);
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] mask_blit with color mask\n", channel_width, has_alpha_tag);
        img.mask_blit(img, 80, 105, 80, 80, 5, 5, 0x20, 0x20, 0x20);
      }

      {
        fwrite_fmt(stderr, "-- [Image:{}-bit/{}] copy\n", channel_width, has_alpha_tag);
        Image img2(img);
        expect_eq(img, img2);
      }

      vector<Image::Format> formats({Image::Format::COLOR_PPM});
      if (channel_width == 8) {
        formats.emplace_back(Image::Format::WINDOWS_BITMAP);
      }

      for (auto format : formats) {
        const char* ext = Image::file_extension_for_format(format);
        string reference_filename = std::format("reference/ImageTestReference.{}.{}.{}", has_alpha_tag, channel_width, ext);
        string temp_filename = std::format("ImageTestImage.{}.{}.{}", has_alpha_tag, channel_width, ext);

        string reference_data = isfile(reference_filename) ? load_file(reference_filename) : "";
        if (reference_data.empty()) {
          fwrite_fmt(stderr, "warning: reference file {} not found; skipping verification\n", reference_filename);
        }

        {
          fwrite_fmt(stderr, "-- [Image:{}-bit/{}/{}] save to disk\n", channel_width, has_alpha_tag, ext);
          img.save(temp_filename.c_str(), format);
          if (!reference_data.empty()) {
            expect_eq(reference_data, load_file(temp_filename));
          }
        }

        {
          fwrite_fmt(stderr, "-- [Image:{}-bit/{}/{}] save in memory\n", channel_width, has_alpha_tag, ext);
          string data = img.save(format);
          if (!reference_data.empty()) {
            expect_eq(reference_data, data);
          }
        }

        {
          fwrite_fmt(stderr, "-- [Image:{}-bit/{}/{}] compare with saved image\n", channel_width, has_alpha_tag, ext);
          Image ref(temp_filename);
          expect_eq(ref, img);
        }

        if (!reference_data.empty()) {
          fwrite_fmt(stderr, "-- [Image:{}-bit/{}/{}] compare with reference\n", channel_width, has_alpha_tag, ext);
          Image ref(reference_filename);
          expect_eq(ref, img);
        }

        unlink(temp_filename);
      }
    }
  }

  {
    BitmapImage bm(202, 206);

    {
      fwrite_fmt(stderr, "-- [BitmapImage] metadata\n");
      expect_eq(202, bm.get_width());
      expect_eq(206, bm.get_height());
    }

    {
      fwrite_fmt(stderr, "-- [BitmapImage] clear\n");
      for (size_t y = 0; y < bm.get_height(); y++) {
        for (size_t x = 0; x < bm.get_width(); x++) {
          expect_eq(false, bm.read_pixel(x, y));
        }
      }
      bm.clear(true);
      for (size_t y = 0; y < bm.get_height(); y++) {
        for (size_t x = 0; x < bm.get_width(); x++) {
          expect_eq(true, bm.read_pixel(x, y));
        }
      }
      bm.clear(false);
      for (size_t y = 0; y < bm.get_height(); y++) {
        for (size_t x = 0; x < bm.get_width(); x++) {
          expect_eq(false, bm.read_pixel(x, y));
        }
      }
    }

    {
      fwrite_fmt(stderr, "-- [BitmapImage] read/write pixels\n");
      expect_eq(false, bm.read_pixel(0, 0));
      expect_eq(false, bm.read_pixel(1, 1));
      expect_eq(false, bm.read_pixel(101, 101));
      bm.write_pixel(1, 1, true);
      expect_eq(false, bm.read_pixel(0, 0));
      expect_eq(true, bm.read_pixel(1, 1));
      expect_eq(false, bm.read_pixel(101, 101));
      bm.write_pixel(101, 101, true);
      expect_eq(false, bm.read_pixel(0, 0));
      expect_eq(true, bm.read_pixel(1, 1));
      expect_eq(true, bm.read_pixel(101, 101));
    }

    {
      fwrite_fmt(stderr, "-- [BitmapImage] write pattern\n");
      for (size_t y = 0; y < bm.get_height(); y++) {
        for (size_t x = 0; x < bm.get_width(); x++) {
          bm.write_pixel(x, y, (x & y) & 1);
        }
      }
      for (size_t y = 0; y < bm.get_height(); y++) {
        for (size_t x = 0; x < bm.get_width(); x++) {
          expect_eq((x & y) & 1, bm.read_pixel(x, y));
        }
      }
    }

    {
      fwrite_fmt(stderr, "-- [BitmapImage] copy\n");
      BitmapImage bm2(bm);
      expect_eq(bm, bm2);
    }

    {
      string reference_filename = "reference/BitmapImageTestReference.bmp";
      string temp_filename = "BitmapImageTestImage.bmp";

      string reference_data;
      if (isfile(reference_filename)) {
        reference_data = load_file(reference_filename);
      } else {
        fwrite_fmt(stderr, "warning: reference file {} not found; skipping verification\n", reference_filename);
      }

      Image img;
      {
        fwrite_fmt(stderr, "-- [BitmapImage] convert to Image\n");

        img = bm.to_color(0x400000E0, 0x00C00080, false);
        expect_eq(img.get_has_alpha(), false);
        expect_eq(0x400000FF, img.read_pixel(0, 0));
        expect_eq(0x00C000FF, img.read_pixel(1, 1));

        img = bm.to_color(0x400000E0, 0x00C00080, true);
        expect_eq(img.get_has_alpha(), true);
        expect_eq(0x400000E0, img.read_pixel(0, 0));
        expect_eq(0x00C00080, img.read_pixel(1, 1));
      }

      {
        fwrite_fmt(stderr, "-- [BitmapImage] save\n");
        img.save(temp_filename.c_str(), Image::Format::WINDOWS_BITMAP);
        if (!reference_data.empty()) {
          expect_eq(reference_data, load_file(temp_filename));
        }
      }

      if (!reference_data.empty()) {
        fwrite_fmt(stderr, "-- [BitmapImage] compare with reference\n");
        Image ref(reference_filename);
        expect_eq(ref, img);
      }

      unlink(temp_filename);
    }
  }

  fwrite_fmt(stdout, "ImageTest: all tests passed\n");
  return 0;
}
