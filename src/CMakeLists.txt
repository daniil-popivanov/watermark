add_library(wmarkplacer get_data.cpp ../include/wmarkplacer/get_data.h mark_building.cpp ../include/wmarkplacer/mark_building.hpp)

target_include_directories(wmarkplacer PUBLIC "${PROJECT_SOURCE_DIR}/include")
