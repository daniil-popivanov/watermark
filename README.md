# Watermark

## Architecture

```
.
├── internal
│   ├── get_data.cpp      # read and parse bytes from *.bmp file logic
│   └── mark_building.cpp # watermark building logic
├── tests
│   ├── testdata          # contain data for tests
│   ├── CMakeLists.txt
│   └── test_generator.py # generate tests for each block
├── CMakeLists.txt
├── main.cpp              # realise program logic
└── README.md
```