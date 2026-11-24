#include <cstdint>
#include <iostream>
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

    image.Read(argv[1]);

    mark.Read(argv[2]);

    if (!ApplyWatermark(image, mark)){
        return 1;
    }

    image.Create(argv[3], pixels_per_meter);

    return 0;
}
