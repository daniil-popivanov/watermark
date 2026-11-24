#include <cstddef>
#include <cstdint>
#include <iostream>

uint8_t BlendChannel(uint8_t orig, uint8_t mark, double alpha = 0.5) {
  if (alpha < 0.0) {
    alpha = 0.0;
  } else if (alpha > 1.0) {
    alpha = 1.0;
  }

  double blended = orig * (1.0 - alpha) + mark * alpha + 0.5;
  if (blended < 0.0) {
    return 0;
  }
  if (blended > 255.0) {
    return 255;
  }
  return static_cast<uint8_t>(blended);
}

bool ApplyWatermarkToGrid(uint8_t* base_grid, std::size_t base_w,
                          std::size_t base_h, const uint8_t* watermark_grid,
                          std::size_t mark_w, std::size_t mark_h,
                          double alpha = 0.35, bool center = true) {
  if (base_grid == nullptr || base_w == 0 || base_h == 0) {
    std::cerr << "Base grid is empty or invalid\n";
    return false;
  }
  if (watermark_grid == nullptr || mark_w == 0 || mark_h == 0) {
    std::cerr << "Watermark grid is empty or invalid\n";
    return false;
  }

  if (alpha < 0.0) {
    alpha = 0.0;
  } else if (alpha > 1.0) {
    alpha = 1.0;
  }

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

  constexpr std::size_t kColorsCount = 3;  
  
  for (std::size_t y = 0; y < mark_h; ++y) {
    std::size_t target_y = start_y + y;
    if (target_y >= base_h) break;

    for (std::size_t x = 0; x < mark_w; ++x) {
      std::size_t target_x = start_x + x;
      if (target_x >= base_w) break;

      std::size_t base_idx = (target_y * base_w + target_x) * kColorsCount;
      std::size_t mark_idx = (y * mark_w + x) * kColorsCount;

      for (std::size_t c = 0; c < kColorsCount; ++c) {
        base_grid[base_idx + c] = BlendChannel(
            base_grid[base_idx + c], watermark_grid[mark_idx + c], alpha);
      }
    }
  }

  return true;
}