#define _STDC_FORMAT_MACROS
#include <inttypes.h>
#include <assert.h>
#include <sys/time.h>

#include <vector>

#include "Filesystem.hh"
#include "Image.hh"
#include "Strings.hh"
#include "UnitTest.hh"

using namespace std;



struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) { }
};

static const vector<Color> colors({
  Color(0xFF, 0x00, 0x00),
  Color(0xFF, 0x80, 0x00),
  Color(0xFF, 0xFF, 0x00),
  Color(0x00, 0xFF, 0x00),
  Color(0x00, 0xFF, 0xFF),
  Color(0x00, 0x00, 0xFF),
  Color(0xFF, 0x00, 0xFF),
  Color(0xFF, 0xFF, 0xFF),
});

int main(int, char** argv) {

  for (uint8_t channel_width = 8; channel_width <= 64; channel_width <<= 1) {
    Image img(180, 190, false, channel_width);

    {
      fprintf(stderr, "-- [%hhu-bit] metadata\n", channel_width);
      expect_eq(180, img.get_width());
      expect_eq(190, img.get_height());
      expect_eq(false, img.get_has_alpha());
      expect_eq(channel_width, img.get_channel_width());
    }

    {
      fprintf(stderr, "-- [%hhu-bit] clear\n", channel_width);
      img.clear(0x20, 0x20, 0x20);
    }

    {
      fprintf(stderr, "-- [%hhu-bit] axis-aligned lines\n", channel_width);
      for (size_t x = 0; x < 8; x++) {
        const auto& c = colors[x];
        img.draw_horizontal_line(5, 175, 90 + x, x, c.r, c.g, c.b);
        img.draw_vertical_line(5 + x, 100, 185, x, c.r, c.g, c.b);
      }
    }

    {
      fprintf(stderr, "-- [%hhu-bit] rectangles\n", channel_width);
      img.fill_rect(3, 98, 64, 64, 0xFF, 0xFF, 0xFF, 0x80);
    }

    {
      fprintf(stderr, "-- [%hhu-bit] non-axis-aligned lines\n", channel_width);
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
            points[x + 8].first + 5, points[x + 8].second + 5, c.r, c.g, c.b);
        img.draw_line(points[x + 8].first + 90, points[x + 8].second + 5,
            points[x].first + 90, points[x].second + 5, c.r, c.g, c.b);
      }
    }

    {
      fprintf(stderr, "-- [%hhu-bit] blit\n", channel_width);
      img.blit(img, 40, 105, 80, 80, 5, 5);
    }

    {
      fprintf(stderr, "-- [%hhu-bit] mask_blit with color mask\n", channel_width);
      img.mask_blit(img, 80, 105, 80, 80, 5, 5, 0x20, 0x20, 0x20);
    }

    {
      fprintf(stderr, "-- [%hhu-bit] copy\n", channel_width);
      Image img2(img);
      expect_eq(img, img2);
    }

    vector<Image::Format> formats({Image::Format::COLOR_PPM});
    if (channel_width == 8) {
      formats.emplace_back(Image::Format::WINDOWS_BITMAP);
    }

    for (auto format : formats) {
      const char* ext = Image::file_extension_for_format(format);
      string reference_filename = string_printf("reference/ImageTestReference%hhu.%s", channel_width, ext);
      string temp_filename = string_printf("ImageTestImage%hhu.%s", channel_width, ext);

      string reference_data = isfile(reference_filename) ? load_file(reference_filename) : "";
      if (reference_data.empty()) {
        fprintf(stderr, "warning: reference file %s not found; skipping verification\n",
            reference_filename.c_str());
      }

      {
        fprintf(stderr, "-- [%hhu-bit/%s] save in memory\n", channel_width, ext);
        string data = img.save(format);
        if (!reference_data.empty()) {
          expect_eq(reference_data, data);
        }
      }

      {
        fprintf(stderr, "-- [%hhu-bit/%s] save to disk\n", channel_width, ext);
        img.save(temp_filename.c_str(), format);
        if (!reference_data.empty()) {
          expect_eq(reference_data, load_file(temp_filename));
        }
      }

      {
        fprintf(stderr, "-- [%hhu-bit/%s] compare with saved image\n", channel_width, ext);
        Image ref(temp_filename);
        expect_eq(ref, img);
      }

      if (!reference_data.empty()) {
        fprintf(stderr, "-- [%hhu-bit/%s] compare with reference\n", channel_width, ext);
        Image ref(reference_filename);
        expect_eq(ref, img);
      }

      unlink(temp_filename);
    }
  }

  printf("%s: all tests passed\n", argv[0]);
  return 0;
}
