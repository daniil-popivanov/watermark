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

void ReverseRow(std::vector <ColorPixel> &pixels, int l, int r) {
    for (int i = 0; i < (r - l) / 2; i++) {
        ColorPixel tmp = pixels[l + i];
        pixels[l + i] = pixels[r - (i + 1)];
        pixels[r - (i + 1)] = tmp;
    }
}

class BMPData {
    private:
        int32_t width = 0;  // width of picture
        int32_t height = 0;  // height of picture
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
            std::fread(&offset, 4, 1, file);

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
            std::memcpy(&(*this).height, &info_header[8], 4);
            std::memcpy(&(*this).bit_depth, &info_header[14], 2);
            std::fseek(file, offset, SEEK_SET);    // relocate pointer to first pixel

            uint32_t pixels_in_row = abs(this->width); // pixels in row
            uint64_t pixel_pos = 0;  // point pixel which are proccessing
            uint32_t padding_in_row = 0;
            bool is_bottom_up = false;

            this->pixels = std::vector<ColorPixel> (pixels_in_row * abs((*this).height));
            std::vector<ColorPixel>& color_pixels = this->pixels;
            uint8_t pixel[3];
            bytes_read = fread(&pixel, 1, 3, file);
            uint32_t pixels_read = 0;

            if ((pixels_in_row * 3) % 4 != 0) {   // counting padding bytes in a row
                padding_in_row = 4 - ((pixels_in_row * 3) % 4);
            }

            if ((*this).height > 0) {
                is_bottom_up = true;
            }

            if (is_bottom_up) {
                pixel_pos = pixels_in_row * (*this).height - 1;
            }

            while (bytes_read == 3) {   // read pixels until the end
                ColorPixel cur_pixel = {pixel[0], pixel[1], pixel[2]};
                color_pixels[pixel_pos] = cur_pixel;
                pixels_read++;

                if (pixels_read == pixels_in_row) {
                    if (is_bottom_up) {
                        ReverseRow(color_pixels, pixel_pos, pixel_pos + pixels_in_row);
                    }
                    
                    pixels_read = 0;
                    
                    std::fseek(file, padding_in_row, SEEK_CUR);
                }
                
                if (is_bottom_up) {
                    pixel_pos--;
                } else {
                    pixel_pos++;
                }

                bytes_read = fread(&pixel, 1, 3, file);
            }

            (*this).pixels = color_pixels;
            fclose(file);
            return;
        };
        void Create(const char* filename, uint32_t ppm) { // method create .bmp file with parameters from class
            uint32_t headers_size = 14 + 40;
            uint32_t pallet_size = 0;

            if (this->height > 0) {
                this->height = -this->height;
            }

            if ((*this).bit_depth <= 8) {
                pallet_size = 4 * (int) pow(2, (*this).bit_depth);
            }

            uint32_t row_size = (((*this).bit_depth * abs((*this).width) + 31) / 32) * 4;
            uint32_t file_size = headers_size + pallet_size + row_size * abs((*this).height);
            uint32_t pixels_in_row = abs(this->width);
            uint32_t offset = headers_size + pallet_size;
            std::vector<uint8_t> new_file_bytes(file_size, 0);

            new_file_bytes[0] = 0x42;
            new_file_bytes[1] = 0x4D;
            new_file_bytes[14] = 0x28;
            new_file_bytes[26] = 1;

            for (int i = 0; i < 4; i++) {
                new_file_bytes[2 + i] = (file_size >> 8 * i) & 0xFF;
                new_file_bytes[10 + i] = (offset >> 8 * i) & 0xFF;
                new_file_bytes[18 + i] = ((*this).width >> 8 * i) & 0xFF;
                new_file_bytes[22 + i] = ((*this).height >> 8 * i) & 0xFF;
                new_file_bytes[38 + i] = (ppm >> 8 * i) & 0xFF;
                new_file_bytes[42 + i] = (ppm >> 8 * i) & 0xFF;
            }

            for (int i = 0; i < 2; i++) {
                new_file_bytes[28 + i] = ((*this).bit_depth >> 8 * i) & 0xFF;
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
                }
            }

            for (int row = 0; row < abs(this->height); row++) {
                for (int cur_pixel = 0; cur_pixel < pixels_in_row; cur_pixel++) {
                    new_file_bytes[offset + row_size * row + cur_pixel * 3] = (*this).pixels[pixels_in_row * row + cur_pixel].blue;
                    new_file_bytes[offset + row_size * row + cur_pixel * 3 + 1] = (*this).pixels[pixels_in_row * row + cur_pixel].green;
                    new_file_bytes[offset + row_size * row + cur_pixel * 3 + 2] = (*this).pixels[pixels_in_row * row + cur_pixel].red;
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
        void SetParameters(int32_t width, int32_t height, uint16_t bit_depth, std::vector<ColorPixel> &pixels) {
            if (height > 0) {
                height = -height;
            }

            this->width = abs(width);
            this->height = height;
            this->bit_depth = bit_depth;
            this->pixels = pixels;
        }
        int32_t GetWidth() { return this->width; };
        int32_t GetHeight() { return this->height; };
};