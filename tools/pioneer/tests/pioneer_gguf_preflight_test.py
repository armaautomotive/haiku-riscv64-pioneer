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


def fixture(alignment=32, offset=0, kind=0, dimension=4, duplicate=False, arch="qwen35", shape=None):
    pairs = [("general.architecture", 8, string(arch)),
             ("general.alignment", 4, struct.pack("<I", alignment)),
             (arch + ".embedding_length", 4, struct.pack("<I", 2048)),
             ("tokenizer.chat_template", 8, string("Untrusted content is not evaluated or printed"))]
    if duplicate:
        pairs.append(pairs[0])
    result = b"GGUF" + struct.pack("<IQQ", 3, 1, len(pairs))
    for key, value_type, value in pairs:
        result += string(key) + struct.pack("<I", value_type) + value
    dims = shape or (dimension,)
    result += string("blk.0.weight") + struct.pack("<I", len(dims))
    result += b"".join(struct.pack("<Q", value) for value in dims) + struct.pack("<IQ", kind, offset)
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

    def test_qwen3_matrix_inventory_not_execution(self):
        result = module.inspect(fixture(arch="qwen3", kind=8, shape=(1024, 64)), 100_000)
        self.assertEqual(result["metadata"]["qwen3.embedding_length"], 2048)
        self.assertNotIn("tokenizer.chat_template", result["metadata"])
        self.assertEqual(result["matrix_shapes"], {"Q8_0:1024x64": 1})
        self.assertEqual(result["q8_packed_matrix_bytes"], 73728)
        self.assertTrue(result["header_only_not_runnable"])

    def test_all_tensor_inventory_is_explicit_and_excludes_templates(self):
        self.assertNotIn("tensors", module.inspect(fixture(), 1024))
        result = module.inspect(fixture(), 1024, include_tensors=True)
        self.assertEqual(len(result["tensors"]), 1)
        self.assertEqual(result["tensors"], result["selected_tensors"])
        self.assertNotIn("tokenizer.chat_template", result["metadata"])


if __name__ == "__main__":
    unittest.main()
