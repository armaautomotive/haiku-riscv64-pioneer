#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import copy
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import tenstorrent_qwen38_plan as module


def inventory():
    # Synthetic metadata/data extents,never allocates a model-sized buffer.
    metadata = dict(zip(("general.architecture", "qwen35.block_count", "qwen35.embedding_length",
        "qwen35.feed_forward_length", "qwen35.attention.head_count", "qwen35.attention.head_count_kv",
        "qwen35.attention.key_length", "qwen35.attention.value_length", "qwen35.ssm.conv_kernel",
        "qwen35.ssm.state_size", "qwen35.ssm.group_count", "qwen35.ssm.time_step_rank",
        "qwen35.ssm.inner_size", "qwen35.full_attention_interval", "qwen35.rope.dimension_count"),
        ("qwen35", 64, 5120, 17408, 24, 4, 256, 256, 4, 128, 16, 48, 6144, 4, 64)))
    cursor, tensors = 4096, []
    for name, (shape, kind) in module.expected_tensors().items():
        block, block_bytes = module.FORMATS[kind]
        size = shape[0] * (shape[1] if len(shape) == 2 else 1) // block * block_bytes
        tensors.append(dict(name=name, shape=list(shape), format=kind, bytes=size, file_offset=cursor))
        cursor = (cursor + size + 31) // 32 * 32
    return dict(metadata=metadata, tensors=tensors, data_start=4096, tensor_bytes=sum(t["bytes"] for t in tensors))


class PlanTests(unittest.TestCase):
    def test_all_rows_bytes_banks_bounds_and_no_execution(self):
        source = inventory()
        for context in (1, 8192, 32768):
            result = module.plan(source, context)
            self.assertFalse(result["device_access"] or result["executable_by_installed_backend"] or result["route_or_allocation_authority"])
            self.assertEqual(result["state_bytes"], dict(delta_fp32=150994944, convolution_fp32=5898240, kv_fp16=16*context*4096))
            ends, fragments = [0x04000000] * 7, {}
            for part in result["fragments"]:
                bank, address, allocated = part["bank_index"], part["bank_address"], part["allocated_bytes"]
                self.assertEqual(address, ends[bank])
                self.assertFalse(address % 4096 or allocated % 4096)
                self.assertLessEqual(part["bytes"], allocated)
                self.assertLess(allocated-part["bytes"], 4096)
                ends[bank] += allocated
                self.assertLessEqual(ends[bank], 0x04000000+3*1024**3)
                if part["kind"] == "packed_weight":
                    self.assertFalse(part["zero_initialization_required"])
                    fragments.setdefault(part["name"], []).append(part)
                else:
                    self.assertTrue(part["zero_initialization_required"])
                    self.assertIsNone(part["source_offset"])
            payload = 0
            for tensor in source["tensors"]:
                first = 0
                parts = sorted(fragments[tensor["name"]], key=lambda p: p["first_row"])
                for part in parts:
                    self.assertEqual(part["first_row"], first)
                    self.assertEqual(part["source_offset"], tensor["file_offset"]+first*part["row_bytes"])
                    self.assertEqual(part["bytes"], part["rows"]*part["row_bytes"])
                    self.assertEqual(part["format"], tensor["format"])
                    first += part["rows"]
                    payload += part["bytes"]
                rows = tensor["shape"][1] if len(tensor["shape"]) == 2 else 1
                self.assertEqual(first, rows)
                if rows >= 7*64:
                    self.assertEqual({p["bank_index"] for p in parts}, set(range(7)))
                    self.assertLessEqual(max(p["rows"] for p in parts)-min(p["rows"] for p in parts), 127)
            self.assertEqual(len(fragments), 851)
            self.assertEqual(payload, source["tensor_bytes"])
            self.assertEqual([e-0x04000000 for e in ends], result["allocated_bytes_per_bank"])

    def test_every_tensor_geometry_and_format_fail_closed(self):
        source = inventory()
        for index in range(851):
            original = source["tensors"][index]
            for key, value in (("shape", [1]), ("format", "F16"), ("name", "unknown"),
                               ("bytes", original["bytes"]+1), ("file_offset", 0), ("file_offset", module.FULL_BYTES)):
                source["tensors"][index] = dict(original, **{key: value})
                with self.assertRaises(ValueError):
                    module.plan(source)
            source["tensors"][index] = original

    def test_counts_duplicates_metadata_overlap_capacity_and_context(self):
        source = inventory()
        for key in source["metadata"]:
            invalid = copy.deepcopy(source)
            invalid["metadata"][key] = None
            with self.assertRaises(ValueError):
                module.plan(invalid)
        for transform in (lambda t: t[:-1], lambda t: t+[t[0]], lambda t: [t[0]]+t[:-1]):
            with self.assertRaises(ValueError):
                module.plan(dict(source, tensors=transform(source["tensors"])))
        for context in (0, -1, 262145, 1.5):
            with self.assertRaises(ValueError):
                module.plan(source, context=context)
        for capacity in (2*1024**3, 0, 1, 0x100000000):
            with self.assertRaises(ValueError):
                module.plan(source, capacity=capacity)
        with self.assertRaises(ValueError):
            module.plan(source, context=262144)
        with self.assertRaises(ValueError):
            module.plan(source, workspace_per_bank=1)


if __name__ == "__main__":
    unittest.main()
