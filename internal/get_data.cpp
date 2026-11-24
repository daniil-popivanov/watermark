#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cmath>
#include <filesystem>

enum {
    kBitmapInfoHeaderSize = 40,
    kBitmapInfoHeaderSizeOffset =14,
    kInfoHeaderBitCountOffset = 14,
    kInfoHeaderWidthOffset = 4,
    kInfoHeaderHeightOffset = 8,

    kRowAlign = 4,
    kColorsCount = 3,
    kBluePos = 0,
    kGreenPos = 1,
    kRedPos = 2,
    
    kBitmapFileHeader = 14,
    kFileHeaderSizeOffset = 2,
    kFileHeaderDataOffset = 10,
    kFileWidthOffset = 18,
    kFileHeightOffset = 22,
    kFilePlanesOffset = 26,
    kFileBitCountOffset = 28,

    kPlanesCount = 1,
    kXPpmOffset = 38,
    kYPpmOffset = 42,
    kByteSize = 8,
    kLastByteMask = 0b11111111,
};

enum class BMPErrors {
    kCannotOpenFile = -1,
    kWrongInput = -2,
    kFilenameNotBMP = -3,
    kCannotCreateFile = -4,
};

const uint16_t kSignature = 0x4D42;

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
        void Read(const char* filename) { // method reads .bmp file
            FILE* file = fopen(filename, "rb");
            
            if (!file) {
                std::cout << "Cannot open file!" << std::endl;
                exit((int) BMPErrors::kCannotOpenFile);
            }
            
            uint32_t offset = 0;    // space before first pixel data from first byte
            std::fseek(file, kFileHeaderDataOffset, SEEK_SET);
            std::fread(&offset, 4, 1, file);

            if (std::fseek(file, kBitmapFileHeader, SEEK_SET) != 0) {
                std::cout << "Cannot relocate pointer after fileheader!" << std::endl;
                std::fclose(file);
                exit((int) BMPErrors::kWrongInput);
            }

            uint8_t info_header[kBitmapInfoHeaderSize];
            size_t bytes_read = fread(&info_header, 1, kBitmapInfoHeaderSize, file);

            if (bytes_read < kBitmapInfoHeaderSize) {
                std::cout << "Wrong BITMAPINFOHEADER" << std::endl;
                std::fclose(file);
                exit((int) BMPErrors::kWrongInput);
            }

            std::memcpy(&(*this).width_, &info_header[kInfoHeaderWidthOffset], sizeof(int32_t));   // getting width, higth and bit_depth from INFOHEADER
            std::memcpy(&(*this).height_, &info_header[kInfoHeaderHeightOffset], sizeof(int32_t));
            std::memcpy(&(*this).bit_depth_, &info_header[kInfoHeaderBitCountOffset], sizeof(uint16_t));
            std::fseek(file, offset, SEEK_SET);    // relocate pointer to first pixel

            uint32_t pixels_in_row = abs(this->width_); // pixels in row
            uint32_t padding_in_row = 0;
            bool is_bottom_up = false;

            this->pixels_ = std::vector<ColorPixel> (pixels_in_row * abs((*this).height_));
            std::vector<ColorPixel>& color_pixels = this->pixels_;

            if ((pixels_in_row * kColorsCount) % kRowAlign != 0) {   // counting padding bytes in a row
                padding_in_row = kRowAlign - ((pixels_in_row * kColorsCount) % kRowAlign);
            }

            if ((*this).height_ > 0) {
                is_bottom_up = true;
            }

            const size_t rows_count = abs(this->height_);

            for (size_t rows_read = 0; rows_read < rows_count; rows_read++) { // reading pixels to tablet
                int32_t row = rows_read;

                if (is_bottom_up) {
                    row = rows_count - 1 - row;
                }

                for (size_t pixel = 0; pixel < pixels_in_row; pixel++) {
                    uint8_t pxl[kColorsCount];
                    bytes_read = fread(&pxl, sizeof(uint8_t), kColorsCount, file);

                    if (bytes_read < kColorsCount) {
                        std::fclose(file);
                        exit((int) BMPErrors::kWrongInput);
                    }

                    ColorPixel cur_pixel = {pxl[kBluePos], pxl[kGreenPos], pxl[kRedPos]};
                    color_pixels[row * pixels_in_row + pixel] = cur_pixel;
                }

                std::fseek(file, padding_in_row, SEEK_CUR);
            }

            this->height_ = std::abs(height_);
            this->width_ = std::abs(width_);
            fclose(file);
        };
        void Create(const char* filename, uint32_t ppm) { // method create .bmp file with parameters from class
            std::filesystem::path file_path(filename);

            if (std::strcmp(file_path.extension().c_str(), ".bmp") != 0) { // check for correct extension
                std::cout << "Wrong file extension!" << std::endl;
                exit((int) BMPErrors::kFilenameNotBMP);
            }

            uint32_t headers_size = kBitmapFileHeader + kBitmapInfoHeaderSize;
            uint32_t row_size = (((*this).bit_depth_ * abs((*this).width_) + 31) / 32) * 4;
            uint32_t file_size = headers_size + row_size * abs((*this).height_);
            uint32_t pixels_in_row = abs(this->width_);
            uint32_t offset = headers_size;
            std::vector<uint8_t> new_file_bytes(file_size, 0);

            std::memcpy(new_file_bytes.data(), &kSignature, sizeof(uint16_t));
            new_file_bytes[kBitmapInfoHeaderSizeOffset] = kBitmapInfoHeaderSize;
            new_file_bytes[kFilePlanesOffset] = kPlanesCount;

            // next three 'for' write data to .bmp file

            for (size_t i = 0; i < 4; i++) {
                new_file_bytes[kFileHeaderSizeOffset + i] = (file_size >> kByteSize * i) & kLastByteMask;
                new_file_bytes[kFileHeaderDataOffset + i] = (offset >> kByteSize * i) & kLastByteMask;
                new_file_bytes[kFileWidthOffset + i] = ((*this).width_ >> kByteSize * i) & kLastByteMask;
                new_file_bytes[kFileHeightOffset + i] = ((*this).height_ >> kByteSize * i) & kLastByteMask;
                new_file_bytes[kXPpmOffset + i] = (ppm >> kByteSize * i) & kLastByteMask;
                new_file_bytes[kYPpmOffset + i] = (ppm >> kByteSize * i) & kLastByteMask;
            }

            for (size_t i = 0; i < 2; i++) {
                new_file_bytes[kFileBitCountOffset + i] = ((*this).bit_depth_ >> kByteSize * i) & kLastByteMask;
            }

            const size_t rows_count = std::abs(this->height_);

            for (size_t row = 0; row < rows_count; row++) { // writing pixels to file
                size_t src = rows_count - (1 + row); // takes current pixel for bottom-up

                for (size_t cur_pixel = 0; cur_pixel < pixels_in_row; cur_pixel++) {
                    new_file_bytes[offset + row_size * row + cur_pixel * kColorsCount + kBluePos] = (*this).pixels_[pixels_in_row * src + cur_pixel].blue;
                    new_file_bytes[offset + row_size * row + cur_pixel * kColorsCount + kGreenPos] = (*this).pixels_[pixels_in_row * src + cur_pixel].green;
                    new_file_bytes[offset + row_size * row + cur_pixel * kColorsCount + kRedPos] = (*this).pixels_[pixels_in_row * src + cur_pixel].red;
                }
            }

            FILE* file = fopen(filename, "wb");

            if (!file) {
                std::cout << "Cannot create file!" << std::endl;
                exit((int) BMPErrors::kCannotCreateFile);
            }

            std::fwrite(new_file_bytes.data(), sizeof(uint8_t), file_size, file);
            std::fclose(file);
        };
        void SetParameters(int32_t width, int32_t height, uint16_t bit_depth, const std::vector<ColorPixel> &pixels) { // saving bmp variables inside code
            this->width_ = std::abs(width);
            this->height_ = std::abs(height);
            this->bit_depth_ = bit_depth;
            this->pixels_ = pixels;
        }
        int32_t GetWidth() { return this->width_; }
        int32_t GetHeight() { return this->height_; }
        uint16_t GetBitDepth() { return this->bit_depth_; }
        const std::vector <ColorPixel>& GetPixels() { return this->pixels_; }
};