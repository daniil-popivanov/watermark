#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cassert>

ColorPixel BlendPixels(const ColorPixel& orig, const ColorPixel& mark, double alpha = 0.5) {

    alpha = std::clamp(alpha, 0.0, 1.0);

    int new_blue  = static_cast<int>(orig.blue  * (1.0 - alpha) + mark.blue  * alpha);
    int new_green = static_cast<int>(orig.green * (1.0 - alpha) + mark.green * alpha);
    int new_red   = static_cast<int>(orig.red   * (1.0 - alpha) + mark.red   * alpha);

    return ColorPixel{
        static_cast<uint8_t>(std::clamp(new_blue,  0, 255)),
        static_cast<uint8_t>(std::clamp(new_green, 0, 255)),
        static_cast<uint8_t>(std::clamp(new_red,   0, 255))
    };
}

bool ApplyWatermark(BMPData& base_image, const BMPData& watermark, double alpha = 0.35, bool center = true) {
    int32_t base_w = base_image.GetWidth();
    int32_t base_h = base_image.GetHeight();
    int32_t mark_w = watermark.GetWidth();
    int32_t mark_h = watermark.GetHeight();

    if (base_w <= 0 || base_h <= 0 || base_image.GetPixels().empty()) {
        std::cerr << "Error: Base image is empty or invalid!" << std::endl;
        return false;
    }
    if (mark_w <= 0 || mark_h <= 0 || watermark.GetPixels().empty()) {
        std::cerr << "Error: Watermark image is empty or invalid!" << std::endl;
        return false;
    }

    if (mark_w > base_w || mark_h > base_h) {
        std::cout << "Warning: Watermark is larger than the base image. It will be cropped to fit." << std::endl;
    }

    int32_t start_x = 0;
    int32_t start_y = 0;

    if (center) {
        start_x = std::max(0, (base_w - mark_w) / 2);
        start_y = std::max(0, (base_h - mark_h) / 2);
    } else {
        start_x = std::max(0, base_w - mark_w);
        start_y = std::max(0, base_h - mark_h);
    }

    auto& base_pixels = base_image.GetPixels();
    const auto& mark_pixels = watermark.GetPixels();

    for (int32_t y = 0; y < mark_h; ++y) {
        int32_t target_y = start_y + y;
        if (target_y >= base_h) break;

        for (int32_t x = 0; x < mark_w; ++x) {
            int32_t target_x = start_x + x;
            if (target_x >= base_w) break;

            size_t base_idx = static_cast<size_t>(target_y) * base_w + target_x;
            size_t mark_idx = static_cast<size_t>(y) * mark_w + x;

            base_pixels[base_idx] = BlendPixels(base_pixels[base_idx], mark_pixels[mark_idx], alpha);
        }
    }

    return true;
}