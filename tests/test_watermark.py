import os
from pathlib import Path
import subprocess
import tempfile
import unittest

from tests.test_generator import BITS_PER_PIXEL, BYTES_PER_PIXEL, FILE_HEADER, HEADER_SIZE, INFO_HEADER, ROW_ALIGNMENT, make_valid_bmp


CLI_TIMEOUT = 10
BLACK = bytes((0, 0, 0))
WHITE = bytes((255, 255, 255))


def bmp_from_rows(rows):
    row_bytes = len(rows[0])
    width = row_bytes // BYTES_PER_PIXEL
    padding = bytes(-row_bytes % ROW_ALIGNMENT)
    header = make_valid_bmp(width, len(rows))[:HEADER_SIZE]
    return header + b"".join(row + padding for row in reversed(rows))


class WatermarkTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        executable = os.environ.get("WATERMARK_EXECUTABLE")

        if not executable:
            raise RuntimeError("Run the watermark_cli test through CTest")

        cls.executable = Path(executable).resolve()

    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix="watermark-test-")
        self.addCleanup(directory.cleanup)
        self.work = Path(directory.name)
        self.input = self.work / "input image.bmp"
        self.mark = self.work / "watermark image.bmp"
        self.output = self.work / "output image.bmp"

        self.input.write_bytes(make_valid_bmp(3, 3))
        self.mark.write_bytes(make_valid_bmp())

    def run_watermark(self, *arguments):
        return subprocess.run([str(self.executable), *(str(arg) for arg in arguments)], cwd=self.work, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=CLI_TIMEOUT)

    def assert_image(self, rows):
        data = self.output.read_bytes()
        signature, file_size, _, _, offset = FILE_HEADER.unpack_from(data)
        header_size, width, height, planes, depth, compression, *_ = INFO_HEADER.unpack_from(data, FILE_HEADER.size)
        row_bytes = len(rows[0])
        stride = row_bytes + (-row_bytes) % ROW_ALIGNMENT

        self.assertEqual((signature, file_size, header_size), (b"BM", len(data), INFO_HEADER.size))
        self.assertEqual((width, abs(height), planes, depth, compression), (row_bytes // BYTES_PER_PIXEL, len(rows), 1, BITS_PER_PIXEL, 0))
        self.assertGreaterEqual(offset, HEADER_SIZE)
        self.assertGreaterEqual(len(data), offset + stride * len(rows))

        actual_rows = tuple(data[offset + y * stride:offset + y * stride + row_bytes] for y in range(len(rows)))

        if height > 0:
            actual_rows = actual_rows[::-1]

        self.assertEqual(actual_rows, rows)

    def assert_failure(self, result, message):
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(len(result.stderr.splitlines()), 1, result.stderr)
        self.assertTrue(result.stderr.startswith(message), result.stderr)
        self.assertFalse(self.output.exists())

    def test_centered_watermark(self):
        black_row = BLACK * 3
        self.input.write_bytes(bmp_from_rows((black_row, black_row, black_row)))
        self.mark.write_bytes(bmp_from_rows((WHITE,)))

        res = self.run_watermark(self.input, self.mark, self.output)
        self.assertEqual(res.returncode, 0, res.stderr)
        self.assert_image((black_row, BLACK + bytes((89, 89, 89)) + BLACK, black_row))

    def test_colored_watermark_preserves_rows_and_channels(self):
        background = bytes((100, 100, 100))
        background_row = background * 4
        self.input.write_bytes(bmp_from_rows((background_row,) * 4))
        self.mark.write_bytes(bmp_from_rows((bytes((255, 0, 0, 0, 0, 255)), bytes((0, 255, 0, 255, 255, 255)))))
        top = background + bytes((154, 65, 65, 65, 65, 154)) + background
        bottom = background + bytes((65, 154, 65, 154, 154, 154)) + background

        res = self.run_watermark(self.input, self.mark, self.output)
        self.assertEqual(res.returncode, 0, res.stderr)
        self.assert_image((background_row, top, bottom, background_row))

    def test_missing_arguments(self):
        res = self.run_watermark()
        self.assert_failure(res, "Usage:")

    def test_missing_input(self):
        res = self.run_watermark(self.work / "missing.bmp", self.mark, self.output)
        self.assert_failure(res, "Read input image:")

    def test_missing_watermark(self):
        res = self.run_watermark(self.input, self.work / "missing.bmp", self.output)
        self.assert_failure(res, "Read watermark:")

    def test_missing_output_directory(self):
        self.output = self.work / "missing" / self.output.name

        res = self.run_watermark(self.input, self.mark, self.output)
        self.assert_failure(res, "Write output image:")


if __name__ == "__main__":
    unittest.main()
