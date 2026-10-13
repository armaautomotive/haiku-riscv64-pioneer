#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

sys.dont_write_bytecode = True
path = Path(__file__).resolve().parents[1] / "kexec_probe/verify_image.py"
spec = importlib.util.spec_from_file_location("probe_image", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ImageTest(unittest.TestCase):
    def image(self):
        data = bytearray(24576)
        struct.pack_into("<IIQQQIIQ8sII", data, 0, 0x0400006f, 0x13,
                         0x200000, len(data), 0, 2, 0, 0,
                         b"RISCV\0\0\0", 0x05435352, 0)
        data[64:68] = b"\x13\0\0\0"
        return data

    def test_valid(self):
        self.assertEqual(len(module.verify(self.image())), 64)

    def test_truncated(self):
        for length in range(64):
            with self.subTest(length=length), self.assertRaises(ValueError):
                module.verify(self.image()[:length])

    def test_corrupt_header(self):
        for offset, encoding, value in [
            (0, "I", 0), (4, "I", 0), (8, "Q", 0), (16, "Q", 4096),
            (24, "Q", 1), (32, "I", 1), (36, "I", 1), (40, "Q", 1),
            (48, "Q", 0), (56, "I", 0), (60, "I", 64),
        ]:
            data = self.image()
            struct.pack_into("<" + encoding, data, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                module.verify(data)

    def test_missing_entry(self):
        data = self.image()
        data[64:96] = bytes(32)
        with self.assertRaises(ValueError):
            module.verify(data)

    def test_extra_byte(self):
        with self.assertRaises(ValueError):
            module.verify(self.image() + b"\0")

    def test_oversized(self):
        data = self.image() + bytes(131072)
        struct.pack_into("<Q", data, 16, len(data))
        with self.assertRaises(ValueError):
            module.verify(data)


if __name__ == "__main__":
    unittest.main()
