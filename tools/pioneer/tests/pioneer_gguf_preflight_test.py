#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location("preflight", Path(__file__).resolve().parents[1] / "tenstorrent_gguf_preflight.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def string(value):
    raw = value.encode()
    return struct.pack("<Q", len(raw)) + raw


def fixture(alignment=32, offset=0, kind=0, dimension=4, duplicate=False):
    pairs = [("general.architecture", 8, string("qwen35")),
             ("general.alignment", 4, struct.pack("<I", alignment)),
             ("tokenizer.chat_template", 8, string("Untrusted content is not evaluated or printed"))]
    if duplicate:
        pairs.append(pairs[0])
    result = b"GGUF" + struct.pack("<IQQ", 3, 1, len(pairs))
    for key, value_type, value in pairs:
        result += string(key) + struct.pack("<I", value_type) + value
    result += string("blk.0.weight") + struct.pack("<IQIQ", 1, dimension, kind, offset)
    return result


class PreflightTests(unittest.TestCase):
    def test_valid_header_not_a_runnable_model(self):
        result = module.inspect(fixture(), 1024)
        self.assertTrue(result["header_only_not_runnable"])
        self.assertEqual(result["metadata"]["general.architecture"], "qwen35")
        self.assertNotIn("tokenizer.chat_template", result["metadata"])
        self.assertEqual(result["formats"], {"F32": 1})
        self.assertEqual(result["selected_tensors"][0]["bytes"], 16)

    def test_every_header_truncation_rejected(self):
        raw = fixture()
        for length in range(len(raw)):
            with self.assertRaises(ValueError):
                module.inspect(raw[:length], 1024)

    def test_bad_layout_rejected(self):
        for raw, size in [(fixture(alignment=3), 1024), (fixture(offset=1), 1024),
                          (fixture(kind=999), 1024), (fixture(dimension=0), 1024),
                          (fixture(duplicate=True), 1024), (fixture(), 64),
                          (fixture(kind=12, dimension=4), 1024)]:
            with self.assertRaises(ValueError):
                module.inspect(raw, size)

    def test_q4_block_size(self):
        result = module.inspect(fixture(kind=12, dimension=256), 1024)
        self.assertEqual(result["selected_tensors"][0]["bytes"], 144)


if __name__ == "__main__":
    unittest.main()
