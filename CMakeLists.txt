cmake_minimum_required(VERSION 3.24)

#set(CMAKE_CXX_COMPILER clang++)

project(HamArc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(CMAKE_BUILD_TYPE Debug)
#add_compile_options(-g -fsanitize=address,undefined,leak -fno-omit-frame-pointer)
#add_link_options(-fsanitize=address,undefined,leak)

add_library (wmarkplacer
    src/get_data.cpp
    src/mark_building.cpp
)

target_include_directories(harc PUBLIC ${PROJECT_SOURCE_DIR}/include)
add_subdirectory(bin)
target_link_libraries(hamarc PRIVATE harc)
enable_testing()
add_subdirectory(tests)