#!/usr/bin/env python3
"""Checks for variable-size image extraction and malformed partition rejection."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "extract_bfs_image", Path(__file__).resolve().parents[1] / "extract_bfs_image.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ImageExtractionTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.root = Path(self.directory.name)
        self.image = self.root / "haiku.image"

    def tearDown(self):
        self.directory.cleanup()

    def make_image(self, size_mib=2):
        offset = 4096
        size = size_mib * 1024 * 1024
        mbr = bytearray(512)
        mbr[510:] = b"\x55\xaa"
        mbr[450] = 0xef
        struct.pack_into("<II", mbr, 454, 1, 7)
        mbr[466] = 0xeb
        struct.pack_into("<II", mbr, 470, offset // 512, size // 512)
        header = bytearray(116)
        for position, value in ((32, 0x42465331), (36, 0x42494745),
                                (40, 2048), (44, 11), (68, 0xdd121031),
                                (112, 0x15b6830e)):
            struct.pack_into("<I", header, position, value)
        struct.pack_into("<qq", header, 48, size // 2048, 128)
        with self.image.open("wb") as stream:
            stream.truncate(offset + size)
            stream.write(mbr)
            stream.seek(offset + 512)
            stream.write(header)
            stream.seek(offset + size - 16)
            stream.write(b"END_OF_BFS_DATA!")
        return offset, size

    def test_extracts_exact_filesystem_including_end(self):
        offset, size = self.make_image()
        output = self.root / "payload.bfs"
        self.assertEqual(module.extract(self.image, output), size)
        self.assertEqual(output.read_bytes(), self.image.read_bytes()[offset:])

    def test_768_mib_image_is_not_truncated_to_300(self):
        offset, size = self.make_image(768)
        self.assertEqual(module.geometry(self.image), (offset, size))

    def test_overlapping_partition_rejected(self):
        self.make_image()
        with self.image.open("r+b") as stream:
            stream.seek(458)
            stream.write(struct.pack("<I", 20))
        with self.assertRaisesRegex(ValueError, "Overlapping"):
            module.geometry(self.image)

    def test_truncation_rejected_without_replacing_existing_output(self):
        offset, size = self.make_image()
        with self.image.open("r+b") as stream:
            stream.truncate(offset + size - 1)
        output = self.root / "existing.bfs"
        output.write_bytes(b"keep existing output")
        with self.assertRaisesRegex(ValueError, "boundaries"):
            module.extract(self.image, output)
        self.assertEqual(output.read_bytes(), b"keep existing output")

    def test_output_cannot_be_input_or_a_symlink(self):
        self.make_image()
        with self.assertRaises(ValueError):
            module.extract(self.image, self.image)
        link = self.root / "payload-link"
        link.symlink_to(self.image)
        with self.assertRaises(ValueError):
            module.extract(self.image, link)


if __name__ == "__main__":
    unittest.main()
