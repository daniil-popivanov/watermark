cmake_minimum_required(VERSION 3.24)
project(bmp_test_probe LANGUAGES CXX)

set(BMP_DATA_SOURCE "" CACHE FILEPATH "")
if(NOT EXISTS "${BMP_DATA_SOURCE}")
    message(FATAL_ERROR "BMP_DATA_SOURCE file not found")
endif()
get_filename_component(BMP_DATA_SOURCE "${BMP_DATA_SOURCE}" ABSOLUTE)
file(TO_CMAKE_PATH "${BMP_DATA_SOURCE}" BMP_DATA_SOURCE)

add_executable(bmp_probe bmp_probe.cpp)
target_compile_features(bmp_probe PRIVATE cxx_std_17)
set_target_properties(bmp_probe PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(bmp_probe PRIVATE
    "BMP_DATA_SOURCE=\"${BMP_DATA_SOURCE}\"")
if(MSVC)
    target_compile_options(bmp_probe PRIVATE /utf-8)
    target_compile_definitions(bmp_probe PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(bmp_probe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(bmp_probe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin")
endforeach()
