#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cmath>

struct ColorByte {  // ColorByte contains every color parameter for pixel
    uint8_t blue = 0;
    uint8_t green = 0;
    uint8_t red = 0;
};

class BMPData {
    private:
        int32_t width = 0;  // width of picture
        int32_t hight = 0;  // hight of picture
        int32_t bf_off_bits = 0;    // space before first pixel data from first byte
        int16_t bit_depth = 0;  // color depth
        std::vector<ColorByte> pixels; // data about every pixel
        // временно пихнул вектор, потом поменяем, если его нельзя юзать
    public:
        void Read(const char* filename) { // method reads .bmp file
            FILE* file = fopen(filename, "rb");

            if (!file) {
                std::cout << "Cannot open file!" << std::endl;
                return;
            }

            std::fseek(file, 10, SEEK_SET);
            std::memcpy(&(*this).bf_off_bits, file, 4);

            if (std::fseek(file, 14, SEEK_SET) != 0) {
                std::cout << "Cannot relocate pointer after fileheader!" << std::endl;
                return;
            }

            uint8_t info_header[40];
            size_t bytes_read = fread(&info_header, 1, 40, file);

            if (bytes_read < 40) {
                std::cout << "Wrong BITMAPINFOHEADER" << std::endl;
                return;
            }

            std::memcpy(&(*this).width, &info_header[4], 4);   // getting width, higth and bit_depth from INFOHEADER
            std::memcpy(&(*this).hight, &info_header[8], 4);
            std::memcpy(&(*this).bit_depth, &info_header[14], 2);
            std::fseek(file, (*this).bf_off_bits, SEEK_SET);    // relocate pointer to first pixel

            int pixels_in_row = ((*this).width * (*this).bit_depth + 7) / 8 / 3; // pixels in row
            int pixel_pos = 0;  // point pixel which are proccessing
            int padding_in_row = 0;
            bool is_bottom_up = false;

            std::vector<ColorByte> color_pixels(pixels_in_row * (*this).hight);
            uint8_t pixel[3];
            bytes_read = fread(&pixel, 1, 3, file);

            if (pixels_in_row % 4 != 0) {   // counting padding bytes in a row
                padding_in_row = 4 - (pixels_in_row % 4);
            }

            if ((*this).hight > 0) {
                is_bottom_up = true;
            }

            if (is_bottom_up) {
                pixel_pos = pixels_in_row * (*this).hight - 1;
            }

            while (bytes_read == 3) {   // read pixels until the end
                ColorByte cur_pixel = {pixel[0], pixel[1], pixel[2]};
                color_pixels[pixel_pos] = cur_pixel;

                if (is_bottom_up) {
                    pixel_pos--;
                } else {
                    pixel_pos++;
                }
                
                if (pixel_pos % pixels_in_row == 0) {
                    std::fseek(file, padding_in_row, SEEK_CUR);
                }

                bytes_read = fread(&pixel, 1, 3, file);
            }

            (*this).pixels = color_pixels;
            fclose(file);
            return;
        };
        BMPData Create() { //method reads .bmp file
            int headers_size = 14 + 40;
            int pallet_size = 4 * (int) pow(2, (*this).bit_depth);
            int row_size = (((*this).bit_depth * (*this).width + 31) / 32) * 4;
            uint8_t new_file_bytes[headers_size + pallet_size + row_size * abs((*this).hight)];
        };
};