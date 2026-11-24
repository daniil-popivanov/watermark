#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace bmpformat {
    struct ColorPixel {  // ColorByte contains every color parameter for pixel
        uint8_t blue = 0;
        uint8_t green = 0;
        uint8_t red = 0;
    };

    class BMPData {
        private:
            int32_t width_ = 0;  // width of picture
            int32_t height_ = 0;  // height of picture
            uint16_t bit_depth_ = 0;  // color depth
            std::vector<ColorPixel> pixels_; // data about every pixel
        public:
            void Read(const char* filename); // method reads .bmp file
            void Create(const char* filename, uint32_t ppm); // method create .bmp file with parameters from class
            bool TryRead(const char* filename, std::string& error);
            bool TryCreate(const char* filename, uint32_t ppm, std::string& error);
            void SetParameters(int32_t width, int32_t height, uint16_t bit_depth, const std::vector<ColorPixel> &pixels); // saving bmp variables inside code
            int32_t GetWidth();
            int32_t GetHeight();
            uint16_t GetBitDepth();
            const std::vector <ColorPixel>& GetPixels();
    };
} // namespace bmpformat