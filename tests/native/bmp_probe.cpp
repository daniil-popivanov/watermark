#include "wmarkplacer/get_data.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace allocation_probe {
    constexpr std::size_t limit_bytes = 8U * 1024U * 1024U;
    bool enabled = false;
    bool limited = false;
    std::size_t largest_request = 0;
}

void* operator new(std::size_t count) {
    if (allocation_probe::enabled) {
        allocation_probe::largest_request = std::max(allocation_probe::largest_request, count);
        if (count > allocation_probe::limit_bytes) {
            allocation_probe::limited = true;
            throw std::bad_alloc();
        }
    }
    if (void* result = std::malloc(count == 0 ? 1 : count)) {
        return result;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t count) { return ::operator new(count); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }

struct Observation {
    const char* stage = "none";
    const char* status = "returned";
    bool allocation_limited = false;
    std::size_t largest_request = 0;
};

template <class Action>
Observation observe(const char* stage, Action&& action) {
    Observation result;
    result.stage = stage;
    allocation_probe::limited = false;
    allocation_probe::largest_request = 0;
    allocation_probe::enabled = true;
    try {
        using Return = std::invoke_result_t<Action&>;

        if constexpr (std::is_same_v<Return, bool>) {
            result.status = action() ? "accepted" : "rejected";
        } else {
            static_assert(std::is_void_v<Return>, "Expected void or bool");
            action();
            result.status = "returned";
        }
    } catch (const std::bad_alloc&) {
        result.status = "allocation_failure";
    } catch (const std::exception&) {
        result.status = "exception";
    } catch (...) {
        result.status = "nonstandard_exception";
    }
    allocation_probe::enabled = false;
    result.allocation_limited = allocation_probe::limited;
    result.largest_request = allocation_probe::largest_request;
    return result;
}

bool continued(const Observation& result) {
    const std::string_view status(result.status);
    return !result.allocation_limited && (status == "accepted" || status == "returned");
}

int main(int argc, char** argv) {
    try {
        if (argc < 4) return 2;
        const std::string mode = argv[1];
        bmpformat::BMPData image;
        Observation result;
        if (mode == "read" && argc == 4) {
            result = observe("Read", [&]() { return image.Read(argv[2]); });
        } else if (mode == "roundtrip" && argc == 5) {
            result = observe("Read", [&]() { return image.Read(argv[2]); });
            if (continued(result)) {
                constexpr std::uint32_t pixels_per_meter = 1000;
                result = observe("Create", [&]() { return image.Create(argv[3], pixels_per_meter); });
            }
        } else if ((mode == "set" && argc == 6) || (mode == "save" && argc == 7)) {
            const auto width_value = std::stoll(argv[2]);
            const auto height_value = std::stoll(argv[3]);
            const auto count = std::stoull(argv[4]);
            if (width_value < std::numeric_limits<std::int32_t>::min() || width_value > std::numeric_limits<std::int32_t>::max() || height_value < std::numeric_limits<std::int32_t>::min() || height_value > std::numeric_limits<std::int32_t>::max() || count > 16) return 2;
            const auto width = static_cast<std::int32_t>(width_value);
            const auto height = static_cast<std::int32_t>(height_value);
            std::vector<bmpformat::ColorPixel> pixels(static_cast<std::size_t>(count), bmpformat::ColorPixel{17, 63, 129});
            constexpr std::uint16_t bit_depth = 24;
            result = observe("SetParameters", [&]() { return image.SetParameters(width, height, bit_depth, pixels); });
            if (mode == "save" && continued(result)) {
                constexpr std::uint32_t pixels_per_meter = 1000;
                result = observe("Create", [&]() { return image.Create(argv[5], pixels_per_meter); });
            }
        } else {
            return 2;
        }
        std::ofstream output(argv[argc - 1], std::ios::binary);
        if (!output) return 2;
        output << "{\"stage\":\"" << result.stage << "\",\"status\":\"" << result.status << "\",\"width\":" << image.GetWidth() << ",\"height\":" << image.GetHeight() << ",\"allocation_limited\":" << (result.allocation_limited ? "true" : "false") << ",\"largest_request\":" << result.largest_request << "}\n";
        output.close();
        return output ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
