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
        int32_t offset = 0;    // space before first pixel data from first byte
        int16_t bit_depth = 0;  // color depth
        std::vector<ColorByte> pixels; // data about every pixel
        // временно пихнул вектор, потом поменяем, если его нельзя юзать
        // возможно вынесу сюда ещё несколько параметров, как офсет или ещё что-нибудь, но надо будет подредачить Read()
    public:
        void Read(const char* filename) { // method reads .bmp file
            FILE* file = fopen(filename, "rb");

            if (!file) {
                std::cout << "Cannot open file!" << std::endl;
                return;
            }

            std::fseek(file, 10, SEEK_SET);
            std::memcpy(&(*this).offset, file, 4);

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
            std::fseek(file, (*this).offset, SEEK_SET);    // relocate pointer to first pixel

            int32_t pixels_in_row = ((*this).width * (*this).bit_depth + 7) / 8 / 3; // pixels in row
            int32_t pixel_pos = 0;  // point pixel which are proccessing
            int32_t padding_in_row = 0;
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
        BMPData Create() { // method create .bmp file with parameters from class
            int32_t headers_size = 14 + 40;
            int32_t pallet_size = 4 * (int) pow(2, (*this).bit_depth);
            int32_t row_size = (((*this).bit_depth * (*this).width + 31) / 32) * 4;
            int32_t file_size = headers_size + pallet_size + row_size * abs((*this).hight);
            int32_t offset = headers_size + pallet_size;
            uint8_t new_file_bytes[file_size];

            new_file_bytes[0] = 0x42;
            new_file_bytes[1] = 0x4D;
            new_file_bytes[14] = 0x28;
            new_file_bytes[26] = 1;
            new_file_bytes[27] = 0;

            for (int i = 0; i < 4; i++) {
                new_file_bytes[2 + i] = (file_size >> 8 * i) & 0xFF;
                new_file_bytes[6 + i] = 0;
                new_file_bytes[10 + i] = (offset >> 8 * i) & 0xFF;
                new_file_bytes[18 + i] = ((*this).width >> 8 * i) & 0xFF;
                new_file_bytes[22 + i] = ((*this).hight >> 8 * i) & 0xFF;
                new_file_bytes[30 + i] = 0;
                new_file_bytes[34 + i] = 0;
            }

            for (int i = 0; i < 3; i++) {
                new_file_bytes[15 + i] = 0;
            }

            for (int i = 0; i < 2; i++) {
                new_file_bytes[28] = ((*this).bit_depth >> 8 * i) & 0xFF;
            }

            //осталось заполнить последние 4 поля
        };
};