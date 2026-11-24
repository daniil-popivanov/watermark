#include <cstdint>
#include <iostream>
#include <string>
#include "wmarkplacer/get_data.h"
#include "wmarkplacer/mark_building.hpp"

int main(int argc, char* argv[]){

    if (argc != 4){
        std::cerr << "Usage: watermark <input.bmp> <mark.bmp> <output.bmp>\n";
        return 1;
    }

    bmpformat::BMPData image;
    bmpformat::BMPData mark;
    constexpr std::uint32_t pixels_per_meter = 1000;
    std::string error;

    if (!image.TryRead(argv[1], error)){
        std::cerr << "Read input image: " << argv[1] << ": " << error << '\n';
        return 1;
    }

    if (!mark.TryRead(argv[2], error)){
        std::cerr << "Read watermark: " << argv[2] << ": " << error << '\n';
        return 1;
    }

    if (!ApplyWatermark(image, mark)){
        return 1;
    }

    if (!image.TryCreate(argv[3], pixels_per_meter, error)){
        std::cerr << "Write output image: " << argv[3] << ": " << error << '\n';
        return 1;
    }

    return 0;
}
