find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

add_executable(bmp_probe native/bmp_probe.cpp)
target_link_libraries(bmp_probe PRIVATE wmarkplacer)

add_test(NAME bmp_data COMMAND "${Python3_EXECUTABLE}" -m unittest tests/test_bmp_data.py)

set_tests_properties(bmp_data PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" ENVIRONMENT "BMP_PROBE_EXECUTABLE=$<TARGET_FILE:bmp_probe>;PYTHONDONTWRITEBYTECODE=1;PYTHONUTF8=1" TIMEOUT 600)

add_test(NAME watermark_cli COMMAND "${Python3_EXECUTABLE}" -m unittest tests/test_watermark.py)

set_tests_properties(watermark_cli PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" ENVIRONMENT "WATERMARK_EXECUTABLE=$<TARGET_FILE:${PROJECT_NAME}>;PYTHONDONTWRITEBYTECODE=1;PYTHONUTF8=1" TIMEOUT 120)
