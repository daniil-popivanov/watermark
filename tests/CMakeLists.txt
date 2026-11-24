find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

add_test(
    NAME bmp_data
    COMMAND "${Python3_EXECUTABLE}" -m unittest tests/test_bmp_data.py
)

set_tests_properties(bmp_data PROPERTIES
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1;PYTHONUTF8=1"
    TIMEOUT 600
)
