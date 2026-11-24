#pragma once

#include "get_data.h"

#include <cstddef>
#include <vector>

bmpformat::ColorPixel BlendPixels(const bmpformat::ColorPixel& orig, const bmpformat::ColorPixel& mark, double alpha = 0.5);

bool ApplyWatermarkToGrid(std::vector<bmpformat::ColorPixel>& base_grid, std::size_t base_w, std::size_t base_h, const std::vector<bmpformat::ColorPixel>& watermark_grid, std::size_t mark_w, std::size_t mark_h, double alpha = 0.35, bool center = true);

bool ApplyWatermark(bmpformat::BMPData& base_image, bmpformat::BMPData& watermark_image, double alpha = 0.35, bool center = true);
