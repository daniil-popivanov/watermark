#include "include/wmarkplacer/mark_building.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

double NormalizeAlpha(double alpha) {
  if (alpha < 0.0) {
    return 0.0;
  }
  if (alpha > 1.0) {
    return 1.0;
  }
  return alpha;
}

uint8_t BlendChannel(uint8_t orig, uint8_t mark, double alpha) {
  double blended = orig * (1.0 - alpha) + mark * alpha + 0.5;
  if (blended < 0.0) {
    return 0;
  }
  if (blended > 255.0) {
    return 255;
  }
  return static_cast<uint8_t>(blended);
}

}

bmpformat::ColorPixel BlendPixels(const bmpformat::ColorPixel& orig,
                                  const bmpformat::ColorPixel& mark,
                                  double alpha) {
  alpha = NormalizeAlpha(alpha);

  bmpformat::ColorPixel result{};
  result.blue = BlendChannel(orig.blue, mark.blue, alpha);
  result.green = BlendChannel(orig.green, mark.green, alpha);
  result.red = BlendChannel(orig.red, mark.red, alpha);
  return result;
}

bool ApplyWatermarkToGrid(
    std::vector<bmpformat::ColorPixel>& base_grid, std::size_t base_w,
    std::size_t base_h,
    const std::vector<bmpformat::ColorPixel>& watermark_grid,
    std::size_t mark_w, std::size_t mark_h, double alpha, bool center) {
  if (base_w == 0 || base_h == 0 || base_grid.size() < base_w * base_h) {
    std::cerr << "Base grid is empty or invalid\n";
    return false;
  }
  if (mark_w == 0 || mark_h == 0 || watermark_grid.size() < mark_w * mark_h) {
    std::cerr << "Watermark grid is empty or invalid\n";
    return false;
  }

  alpha = NormalizeAlpha(alpha);

  if (mark_w > base_w || mark_h > base_h) {
    std::cout << "Watermark is larger than the base grid\n";
  }

  std::size_t start_x = 0;
  std::size_t start_y = 0;

  if (center) {
    start_x = (base_w > mark_w) ? (base_w - mark_w) / 2 : 0;
    start_y = (base_h > mark_h) ? (base_h - mark_h) / 2 : 0;
  } else {
    start_x = (base_w > mark_w) ? (base_w - mark_w) : 0;
    start_y = (base_h > mark_h) ? (base_h - mark_h) : 0;
  }

  for (std::size_t y = 0; y < mark_h; ++y) {
    std::size_t target_y = start_y + y;
    if (target_y >= base_h) {
      break;
    }

    for (std::size_t x = 0; x < mark_w; ++x) {
      std::size_t target_x = start_x + x;
      if (target_x >= base_w) {
        break;
      }

      std::size_t base_idx = target_y * base_w + target_x;
      std::size_t mark_idx = y * mark_w + x;

      base_grid[base_idx] =
          BlendPixels(base_grid[base_idx], watermark_grid[mark_idx], alpha);
    }
  }

  return true;
}

bool ApplyWatermark(bmpformat::BMPData& base_image,
                    bmpformat::BMPData& watermark_image, double alpha,
                    bool center) {
  int32_t base_w = base_image.GetWidth();
  int32_t base_h = base_image.GetHeight();
  int32_t mark_w = watermark_image.GetWidth();
  int32_t mark_h = watermark_image.GetHeight();

  if (base_w <= 0 || base_h <= 0 || mark_w <= 0 || mark_h <= 0) {
    std::cerr << "Invalid image dimensions\n";
    return false;
  }

  std::vector<bmpformat::ColorPixel> result_pixels = base_image.GetPixels();

  if (!ApplyWatermarkToGrid(result_pixels, static_cast<std::size_t>(base_w),
                            static_cast<std::size_t>(base_h),
                            watermark_image.GetPixels(),
                            static_cast<std::size_t>(mark_w),
                            static_cast<std::size_t>(mark_h), alpha, center)) {
    return false;
  }

  base_image.SetParameters(base_w, base_h, base_image.GetBitDepth(),
                           result_pixels);
  return true;
}