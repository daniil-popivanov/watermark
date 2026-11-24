import unittest

from tests.bmp_support import BmpRunner
from tests.test_generator import BYTES_PER_PIXEL, FIELD_BIT_DEPTH, FIELD_COMPRESSION, FIELD_DIB_SIZE, FIELD_PIXEL_OFFSET, FIELD_PLANES, FIELD_WIDTH, HEADER_SIZE, INT_MAX, INT_MIN, changed_field, changed_header_field, expected_bmp, make_header, make_valid_bmp


class BmpDataTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bmp = BmpRunner()
        cls.addClassCleanup(cls.bmp.close)

        cls.bmp.build()

    def test_truncated_header(self):
        res = self.bmp.read(make_valid_bmp()[:HEADER_SIZE - 1])
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_truncated_pixel(self):
        res = self.bmp.read(make_valid_bmp()[:HEADER_SIZE + BYTES_PER_PIXEL - 1])
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_missing_padding(self):
        res = self.bmp.read(make_valid_bmp()[:-1])
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_wrong_signature(self):
        res = self.bmp.read(b"ZZ" + make_valid_bmp()[2:])
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_wrong_dib_header_size(self):
        res = self.bmp.read(changed_field(FIELD_DIB_SIZE, "I", 12))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_wrong_planes(self):
        res = self.bmp.read(changed_field(FIELD_PLANES, "H", 0))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_wrong_bit_depth(self):
        res = self.bmp.read(changed_field(FIELD_BIT_DEPTH, "H", 16))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_wrong_compression(self):
        res = self.bmp.read(changed_field(FIELD_COMPRESSION, "I", 1))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_pixel_offset_after_eof(self):
        res = self.bmp.read(changed_field(FIELD_PIXEL_OFFSET, "I", 4096))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_zero_width(self):
        res = self.bmp.read(make_header(0, 1))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_zero_height(self):
        res = self.bmp.read(make_header(1, 0))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_too_few_pixels(self):
        res = self.bmp.set_parameters(2, 2, 3)
        self.assertEqual(res, ("SetParameters", "rejected", 0, 0))

    def test_too_many_pixels(self):
        res = self.bmp.set_parameters(2, 2, 5)
        self.assertEqual(res, ("SetParameters", "rejected", 0, 0))

    def test_minimum_1x1(self):
        res = self.bmp.roundtrip(1, 1)
        self.assertEqual(res, expected_bmp(1, 1))

    def test_valid_512x512(self):
        res = self.bmp.roundtrip(512, 512)
        self.assertEqual(res, expected_bmp(512, 512))

    def test_pixel_count_above_int_max(self):
        res = self.bmp.read(make_header(46341, 46341))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_pixel_count_over_uint32(self):
        res = self.bmp.read(make_header(65536, 65536))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_width_above_int_max(self):
        res = self.bmp.read(changed_header_field(FIELD_WIDTH, "I", INT_MAX + 1))
        self.assertEqual(res, ("Read", "rejected", 0, 0))

    def test_height_int_min(self):
        res = self.bmp.read(make_header(1, INT_MIN))
        self.assertEqual(res, ("Read", "rejected", 0, 0))


if __name__ == "__main__":
    unittest.main()
