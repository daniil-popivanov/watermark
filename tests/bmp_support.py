import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from tests.test_generator import BYTES_PER_PIXEL, FILE_HEADER, HEADER_SIZE, INFO_HEADER, ROW_ALIGNMENT, make_valid_bmp

PROJECT_ROOT = Path(__file__).resolve().parents[1]
CALL_TIMEOUT_SECONDS = 10


def build_probe(build_directory):
    executable = os.environ.get("BMP_PROBE_EXECUTABLE")

    if executable and not os.environ.get("BMP_DATA_SOURCE"):
        return Path(executable)

    source = Path(os.environ.get("BMP_DATA_SOURCE", PROJECT_ROOT / "src/get_data.cpp"))

    if not source.is_absolute():
        source = PROJECT_ROOT / source

    source = source.resolve()

    if not source.is_file():
        raise RuntimeError(f"File not found: {source}")

    cmake = shutil.which("cmake")

    if not cmake:
        raise RuntimeError("CMake not found")

    adapter_source = build_directory / "adapter-source"
    adapter_source.mkdir()

    for name in ("CMakeLists.txt", "bmp_probe.cpp"):
        shutil.copy2(PROJECT_ROOT / "tests/native" / name, adapter_source / name)

    command = [cmake, "-S", str(adapter_source), "-B", str(build_directory), f"-DBMP_DATA_SOURCE={source.as_posix()}", "-DCMAKE_BUILD_TYPE=Release",]

    if os.name == "nt" and not os.environ.get("CMAKE_GENERATOR"):
        compiler = shutil.which("g++")
        make = shutil.which("mingw32-make")

        if compiler and make:
            command.extend(["-G", "MinGW Makefiles", f"-DCMAKE_CXX_COMPILER={compiler}", f"-DCMAKE_MAKE_PROGRAM={make}",])

    for arguments in (command, [cmake, "--build", str(build_directory), "--config", "Release"],):
        completed = subprocess.run(arguments, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60, check=False,)

        if completed.returncode:
            raise RuntimeError("Build failed:\n" + completed.stdout + completed.stderr)

    executable = build_directory / "bin" / ("bmp_probe.exe" if os.name == "nt" else "bmp_probe")

    if not executable.is_file():
        raise RuntimeError(f"Executable not found: {executable}")

    return executable


def rejection_status(result):
    return "rejected" if result["status"] == "exception" else result["status"]


class BmpRunner:
    def __init__(self):
        self.directory = tempfile.TemporaryDirectory(prefix="bmp-native-")
        self.probe = None

    def build(self):
        self.probe = build_probe(Path(self.directory.name))

    def close(self):
        self.directory.cleanup()

    def call(self, work, *arguments):
        result_path = work / "observation.json"
        result_path.unlink(missing_ok=True)
        command = [str(self.probe), *(str(item) for item in arguments), str(result_path)]

        try:
            completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=CALL_TIMEOUT_SECONDS, check=False)

        except subprocess.TimeoutExpired:
            raise AssertionError("Operation timed out") from None

        if completed.returncode != 0:
            raise AssertionError(f"Process exited with code {completed.returncode}\nstdout: {completed.stdout}\nstderr: {completed.stderr}")

        if not result_path.is_file():
            raise AssertionError("Result file not found")

        result = json.loads(result_path.read_text(encoding="utf-8"))

        if result["allocation_limited"]:
            raise AssertionError(f"Allocation exceeds limit: {result['largest_request']} bytes")

        return result

    def read(self, data):

        with tempfile.TemporaryDirectory(prefix="bmp-case-") as directory:
            work = Path(directory)
            source = work / "input.bmp"
            source.write_bytes(data)

            result = self.call(work, "read", source)

            return result["stage"], rejection_status(result), result["width"], result["height"]

    def set_parameters(self, width, height, count):

        with tempfile.TemporaryDirectory(prefix="bmp-case-") as directory:

            result = self.call(Path(directory), "set", width, height, count)

            return result["stage"], rejection_status(result), result["width"], result["height"]

    def roundtrip(self, width, height):

        with tempfile.TemporaryDirectory(prefix="bmp-case-") as directory:
            work = Path(directory)
            source, target = work / "input.bmp", work / "output.bmp"
            source.write_bytes(make_valid_bmp(width, height))

            result = self.call(work, "roundtrip", source, target)

            if not target.is_file():
                raise AssertionError("Output file not found")

            data = target.read_bytes()

            if len(data) < HEADER_SIZE:
                raise AssertionError("Incomplete output header")

            header = FILE_HEADER.unpack_from(data)
            info = INFO_HEADER.unpack_from(data, FILE_HEADER.size)
            offset = header[4]

            row_bytes = width * BYTES_PER_PIXEL
            stride = row_bytes + (-row_bytes) % ROW_ALIGNMENT

            if offset < HEADER_SIZE or len(data) < offset + stride * height:
                raise AssertionError("Incomplete output pixels")

            pixels = b"".join(data[offset + row * stride:offset + row * stride + row_bytes] for row in (range(height) if info[2] < 0 else range(height - 1, -1, -1)))

            return {"stage": result["stage"], "accepted": result["status"] in {"accepted", "returned"}, "dimensions": (result["width"], abs(result["height"])), "signature": header[0], "size_matches": header[1] == len(data), "reserved": header[2:4], "info_size": info[0], "file_dimensions": (info[1], abs(info[2])), "format": info[3:6], "pixels": pixels,}
