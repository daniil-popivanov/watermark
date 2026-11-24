#include "wmarkplacer/get_data.h"

#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace bmpformat {
namespace {

constexpr unsigned kFileHeaderSize = 14;
constexpr unsigned kInfoHeaderSize = 40;
constexpr unsigned kHeaderSize = kFileHeaderSize + kInfoHeaderSize;
constexpr uint16_t kSignature = 0x4D42;
constexpr uint16_t kBitDepth = 24;
constexpr unsigned kAlignment = 4;
constexpr unsigned kByteBits = 8;
constexpr unsigned kSignatureOffset = 0;
constexpr unsigned kFileSizeOffset = 2;
constexpr unsigned kReservedOffset = 6;
constexpr unsigned kDataOffset = 10;
constexpr unsigned kWidthOffset = 18;
constexpr unsigned kHeightOffset = 22;
constexpr unsigned kPlanesOffset = 26;
constexpr unsigned kDepthOffset = 28;
constexpr unsigned kCompressionOffset = 30;
constexpr unsigned kImageSizeOffset = 34;
constexpr unsigned kXResolutionOffset = 38;
constexpr unsigned kYResolutionOffset = 42;

using Header = std::array<uint8_t, kHeaderSize>;
static_assert(sizeof(ColorPixel) == 3);

template<class T>
T ReadField(const Header& header, unsigned offset) {
    uint32_t value = 0;

    for (unsigned i = 0; i < sizeof(T); ++i) {
        value |= static_cast<uint32_t>(header[offset + i]) << (kByteBits * i);
    }

    return static_cast<T>(value);
}

template<class T>
void WriteField(Header& header, unsigned offset, T value) {
    for (unsigned i = 0; i < sizeof(T); ++i) {
        header[offset + i] = static_cast<uint8_t>(value >> (kByteBits * i));
    }
}

uint64_t RowSize(int32_t width) {
    const uint64_t bytes = static_cast<uint64_t>(width) * sizeof(ColorPixel);
    return (bytes + kAlignment - 1) / kAlignment * kAlignment;
}

uint32_t ImageSize(int32_t width, int32_t height, uint16_t depth) {
    if (width <= 0 || height <= 0 || depth != kBitDepth) {
        throw std::invalid_argument("Expected a 24-bit BMP with positive dimensions");
    }

    const uint64_t bytes = RowSize(width) * static_cast<uint32_t>(height);

    if (bytes > std::numeric_limits<uint32_t>::max() - kHeaderSize) {
        throw std::invalid_argument("Image exceeds the BMP size limit");
    }

    return static_cast<uint32_t>(bytes);
}

}

void BMPData::Read(const char* filename) {
    if (filename == nullptr) {
        throw std::invalid_argument("Missing input filename");
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Cannot open BMP file");
    }

    const std::streamoff length = file.tellg();
    Header header{};
    file.seekg(0);

    if (!file.read(reinterpret_cast<char*>(header.data()), header.size())) {
        throw std::runtime_error("Incomplete BMP header");
    }

    if (ReadField<uint16_t>(header, kSignatureOffset) != kSignature || ReadField<uint32_t>(header, kFileHeaderSize) != kInfoHeaderSize) {
        throw std::invalid_argument("Invalid BMP header");
    }

    if (ReadField<uint16_t>(header, kPlanesOffset) != 1 || ReadField<uint32_t>(header, kCompressionOffset) != 0 || ReadField<uint32_t>(header, kReservedOffset) != 0) {
        throw std::invalid_argument("Unsupported BMP format");
    }

    const uint32_t raw_width = ReadField<uint32_t>(header, kWidthOffset);
    const uint32_t raw_height = ReadField<uint32_t>(header, kHeightOffset);
    const bool top_down = raw_height > std::numeric_limits<int32_t>::max();
    const uint32_t rows = top_down ? 0u - raw_height : raw_height;

    if (raw_width > std::numeric_limits<int32_t>::max() || rows > std::numeric_limits<int32_t>::max()) {
        throw std::invalid_argument("Invalid BMP dimensions");
    }

    const auto width = static_cast<int32_t>(raw_width);
    const auto height = static_cast<int32_t>(rows);
    const auto depth = ReadField<uint16_t>(header, kDepthOffset);
    const uint32_t image_size = ImageSize(width, height, depth);
    const auto offset = ReadField<uint32_t>(header, kDataOffset);
    const auto declared_size = ReadField<uint32_t>(header, kImageSizeOffset);

    if (length != ReadField<uint32_t>(header, kFileSizeOffset) || offset < kHeaderSize || static_cast<int64_t>(offset) + image_size > length) {
        throw std::invalid_argument("Invalid BMP file size or pixel offset");
    }

    if (declared_size != 0 && declared_size != image_size) {
        throw std::invalid_argument("Incorrect pixel data size");
    }

    std::vector<ColorPixel> pixels(static_cast<std::size_t>(raw_width) * rows);
    const auto row_bytes = static_cast<std::streamsize>(width) * static_cast<std::streamsize>(sizeof(ColorPixel));
    const auto padding = static_cast<std::streamsize>(RowSize(width)) - row_bytes;
    std::array<char, kAlignment> pad{};
    file.seekg(offset);

    for (uint32_t y = 0; y < rows; ++y) {
        const std::size_t row = top_down ? y : rows - 1 - y;
        auto* dest = reinterpret_cast<char*>(pixels.data() + row * raw_width);

        if (!file.read(dest, row_bytes) || !file.read(pad.data(), padding)) {
            throw std::runtime_error("Incomplete BMP row");
        }
    }

    pixels_.swap(pixels);
    width_ = width;
    height_ = height;
    bit_depth_ = depth;
}

void BMPData::Create(const char* filename, uint32_t ppm) {
    if (filename == nullptr || std::filesystem::path(filename).extension() != ".bmp") {
        throw std::invalid_argument("Expected .bmp output");
    }

    const uint32_t image_size = ImageSize(width_, height_, bit_depth_);

    if (pixels_.size() != static_cast<uint64_t>(width_) * static_cast<uint32_t>(height_)) {
        throw std::invalid_argument("Pixel count does not match dimensions");
    }

    Header header{};
    WriteField(header, kSignatureOffset, kSignature);
    WriteField(header, kFileSizeOffset, uint32_t{kHeaderSize + image_size});
    WriteField(header, kDataOffset, uint32_t{kHeaderSize});
    WriteField(header, kFileHeaderSize, uint32_t{kInfoHeaderSize});
    WriteField(header, kWidthOffset, static_cast<uint32_t>(width_));
    WriteField(header, kHeightOffset, static_cast<uint32_t>(height_));
    WriteField(header, kPlanesOffset, uint16_t{1});
    WriteField(header, kDepthOffset, bit_depth_);
    WriteField(header, kImageSizeOffset, image_size);
    WriteField(header, kXResolutionOffset, ppm);
    WriteField(header, kYResolutionOffset, ppm);

    std::ofstream file(filename, std::ios::binary);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    file.write(reinterpret_cast<const char*>(header.data()), header.size());
    const auto row_bytes = static_cast<std::streamsize>(width_) * static_cast<std::streamsize>(sizeof(ColorPixel));
    const auto padding = static_cast<std::streamsize>(RowSize(width_)) - row_bytes;
    const std::array<char, kAlignment> pad{};

    for (int32_t y = height_; y > 0; --y) {
        const ColorPixel* row = pixels_.data() + static_cast<std::size_t>(y - 1) * static_cast<uint32_t>(width_);
        file.write(reinterpret_cast<const char*>(row), row_bytes);
        file.write(pad.data(), padding);
    }

    file.close();
}

bool BMPData::TryRead(const char* filename, std::string& error) {
    try {
        Read(filename);
        return true;
    } catch (const std::exception& failure) {
        error = failure.what();
        return false;
    }
}

bool BMPData::TryCreate(const char* filename, uint32_t ppm, std::string& error) {
    try {
        Create(filename, ppm);
        return true;
    } catch (const std::exception& failure) {
        error = failure.what();
        return false;
    }
}

void BMPData::SetParameters(int32_t width, int32_t height, uint16_t bit_depth, const std::vector<ColorPixel>& pixels) {
    ImageSize(width, height, bit_depth);

    if (pixels.size() != static_cast<uint64_t>(width) * static_cast<uint32_t>(height)) {
        throw std::invalid_argument("Pixel count does not match dimensions");
    }

    pixels_ = pixels;
    width_ = width;
    height_ = height;
    bit_depth_ = bit_depth;
}

int32_t BMPData::GetWidth() { return width_; }
int32_t BMPData::GetHeight() { return height_; }
uint16_t BMPData::GetBitDepth() { return bit_depth_; }
const std::vector<ColorPixel>& BMPData::GetPixels() { return pixels_; }

}
