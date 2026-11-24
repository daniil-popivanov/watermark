import struct


FILE_HEADER = struct.Struct("<2sIHHI")
INFO_HEADER = struct.Struct("<IiiHHIIiiII")
HEADER_SIZE = FILE_HEADER.size + INFO_HEADER.size

BITS_PER_PIXEL = 24
BYTES_PER_PIXEL = 3
ROW_ALIGNMENT = 4


FIELD_FILE_SIZE = 2
FIELD_IMAGE_SIZE = 34

REFERENCE_PPM = 1000

ORIGIN_COLOR = (17, 63, 129)
CHANNEL_VALUE_COUNT = 256


def make_header(width=1, height=1):
    return bytearray(FILE_HEADER.pack(b"BM", HEADER_SIZE, 0, 0, HEADER_SIZE) + INFO_HEADER.pack(INFO_HEADER.size, width, height, 1, BITS_PER_PIXEL, 0, 0, REFERENCE_PPM, REFERENCE_PPM, 0, 0,))


def reference_pixels(width, height):
    result = bytearray()

    for y in range(height):

        for x in range(width):
            result.extend(((ORIGIN_COLOR[0] + x) % CHANNEL_VALUE_COUNT, (ORIGIN_COLOR[1] + y) % CHANNEL_VALUE_COUNT, (ORIGIN_COLOR[2] + x + y) % CHANNEL_VALUE_COUNT,))

    return bytes(result)


def make_valid_bmp(width=1, height=1):

    if not (1 <= width <= 512 and 1 <= height <= 512):
        raise ValueError("Image size must be between 1 and 512")

    row_bytes = width * BYTES_PER_PIXEL
    padding = (-row_bytes) % ROW_ALIGNMENT

    pixels = reference_pixels(width, height)
    payload = b"".join(pixels[y * row_bytes:(y + 1) * row_bytes] + bytes(padding) for y in range(height - 1, -1, -1))

    header = make_header(width, height)
    struct.pack_into("<I", header, FIELD_FILE_SIZE, HEADER_SIZE + len(payload))
    struct.pack_into("<I", header, FIELD_IMAGE_SIZE, len(payload))

    return bytes(header) + payload


def expected_bmp(width, height):
    return {"stage": "Create", "accepted": True, "dimensions": (width, height), "signature": b"BM", "size_matches": True, "reserved": (0, 0), "info_size": INFO_HEADER.size, "file_dimensions": (width, height), "format": (1, BITS_PER_PIXEL, 0), "pixels": reference_pixels(width, height),}
