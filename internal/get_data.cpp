#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

struct ColorByte {  // ColorByte contains every color parameter for pixel
    int8_t blue = 0;
    int8_t green = 0;
    int8_t red = 0;
};

class BMPData {
    private:
        int32_t width = 0;  // width of picture
        int32_t hight = 0;  // hight of picture
        int16_t bit_depth = 0;  // color depth
        std::vector<ColorByte> pixels; // data about every pixel
        // временно пихнул вектор, потом поменяем
    public:
        BMPData Read(const char* filename) { // method reads .bmp file
            BMPData data;
            FILE* file = fopen(filename, "rb");

            if (!file) {
                std::cout << "Cannot open file!" << std::endl;
                return data;
            }

            if (std::fseek(file, 14, SEEK_SET) != 0) {
                std::cout << "Cannot relocate pointer after fileheader!" << std::endl;
                return data;
            }

            int8_t info_header[40];
            size_t bytes_read = fread(&info_header, 1, 40, file);

            if (bytes_read < 40) {
                std::cout << "Wrong BITMAPINFOHEADER" << std::endl;
                return data;
            }

            std::memcpy(&data.width, &info_header[4], 4);
            std::memcpy(&data.hight, &info_header[8], 4);
            std::memcpy(&data.bit_depth, &info_header[14], 4);
            int8_t pixel[3];
            int pixels_in_row = (data.width * data.bit_depth + 7) / 8;
            bytes_read = fread(&pixel, 1, 3, file);
            int pixel_counter = 0;
            int padding_in_row = 0;
            std::vector<ColorByte> color_pixels(pixels_in_row * data.hight);

            if (pixels_in_row % 4 != 0) {
                padding_in_row = 4 - (pixels_in_row % 4);
            }

            while (bytes_read == 3) {
                ColorByte cur_pixel = {pixel[0], pixel[1], pixel[2]};
                color_pixels[pixel_counter++] = cur_pixel;
                
                if (pixel_counter % (pixels_in_row == 0)) {
                    std::fseek(file, padding_in_row, SEEK_CUR);
                }

                bytes_read = fread(&pixel, 1, 3, file);
            }

            data.pixels = color_pixels;
        };
        BMPData Create(BMPData data) { //method reads .bmp file
            return data;
        };
};