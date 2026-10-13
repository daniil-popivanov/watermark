#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cmath>

struct ColorPixel {  // ColorByte contains every color parameter for pixel
    uint8_t blue = 0;
    uint8_t green = 0;
    uint8_t red = 0;
};

class BMPData {
    private:
        int32_t width = 0;  // width of picture
        int32_t hight = 0;  // hight of picture
        uint16_t bit_depth = 0;  // color depth
        std::vector<ColorPixel> pixels; // data about every pixel
    public:
        void Read(const char* filename) { // method reads .bmp file
            FILE* file = fopen(filename, "rb");
            
            if (!file) {
                std::cout << "Cannot open file!" << std::endl;
                return;
            }
            
            uint32_t offset = 0;    // space before first pixel data from first byte
            std::fseek(file, 10, SEEK_SET);
            std::memcpy(&offset, file, 4);

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
            std::fseek(file, offset, SEEK_SET);    // relocate pointer to first pixel

            uint32_t pixels_in_row = (abs((*this).width) * (*this).bit_depth + 7) / 8 / 3; // pixels in row
            uint32_t pixel_pos = 0;  // point pixel which are proccessing
            uint32_t padding_in_row = 0;
            bool is_bottom_up = false;

            std::vector<ColorPixel> color_pixels(pixels_in_row * (*this).hight);
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
                ColorPixel cur_pixel = {pixel[0], pixel[1], pixel[2]};
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
        void Create(char* filename, uint32_t ppm) { // method create .bmp file with parameters from class
            uint32_t headers_size = 14 + 40;
            uint32_t pallet_size = 0;

            if ((*this).bit_depth <= 8) {
                pallet_size = 4 * (int) pow(2, (*this).bit_depth);
            }

            uint32_t row_size = (((*this).bit_depth * abs((*this).width) + 31) / 32) * 4;
            uint32_t file_size = headers_size + pallet_size + row_size * abs((*this).hight);
            uint32_t pixels_in_row = (abs((*this).width) * (*this).bit_depth + 7) / 8 / 3;
            uint32_t offset = headers_size + pallet_size;
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
                new_file_bytes[38 + i] = (ppm >> 8 * i) & 0xFF;
                new_file_bytes[42 + i] = (ppm >> 8 * i) & 0xFF;
                new_file_bytes[46 + i] = 0;
                new_file_bytes[50 + i] = 0;
            }

            for (int i = 0; i < 3; i++) {
                new_file_bytes[15 + i] = 0;
            }

            for (int i = 0; i < 2; i++) {
                new_file_bytes[28] = ((*this).bit_depth >> 8 * i) & 0xFF;
            }

            if (pallet_size > 0) {
                for (int i = 0; i < pallet_size / 4; i++) {
                    for (int blue = 0; blue < 256; blue++) {
                        for (int green = 0; green < 256; green++) {
                            for (int red = 0; red < 256; red++) {
                                new_file_bytes[54 + i * 4] = blue;
                                new_file_bytes[55 + i * 4] = green;
                                new_file_bytes[56 + i * 4] = red;
                            }
                        }
                    }

                    new_file_bytes[57 + i * 4] = 0;
                }
            }

            for (int row = 0; row < (*this).hight; row++) {
                for (int cur_pixel = 0; cur_pixel < pixels_in_row; cur_pixel++) {
                    new_file_bytes[offset + row_size * row + cur_pixel] = (*this).pixels[pixels_in_row * row + cur_pixel].blue;
                    new_file_bytes[offset + row_size * row + cur_pixel + 1] = (*this).pixels[pixels_in_row * row + cur_pixel].green;
                    new_file_bytes[offset + row_size * row + cur_pixel + 2] = (*this).pixels[pixels_in_row * row + cur_pixel].red;
                }

                for (int padding = 0; padding < row_size - pixels_in_row * 3; padding++) {
                    new_file_bytes[offset + row_size * row + pixels_in_row + padding] = 0;
                }
            }

            FILE* file = fopen(filename, "wb");

            if (!file) {
                std::cout << "Cannot create file!" << std::endl;
                return;
            }

            std::fwrite(&new_file_bytes[0], 1, file_size, file);
            std::fclose(file);
        };
};