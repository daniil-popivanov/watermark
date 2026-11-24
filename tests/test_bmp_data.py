import unittest

from tests.bmp_support import BmpRunner
from tests.test_generator import expected_bmp


class BmpDataTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bmp = BmpRunner()
        cls.addClassCleanup(cls.bmp.close)

        cls.bmp.build()

    def test_too_few_pixels(self):
        res = self.bmp.set_parameters(2, 2, 3)
        self.assertEqual(res, ("SetParameters", "rejected"))

    def test_too_many_pixels(self):
        res = self.bmp.set_parameters(2, 2, 5)
        self.assertEqual(res, ("SetParameters", "rejected"))

    def test_minimum_1x1(self):
        res = self.bmp.roundtrip(1, 1)
        self.assertEqual(res, expected_bmp(1, 1))

    def test_valid_512x512(self):
        res = self.bmp.roundtrip(512, 512)
        self.assertEqual(res, expected_bmp(512, 512))


if __name__ == "__main__":
    unittest.main()
