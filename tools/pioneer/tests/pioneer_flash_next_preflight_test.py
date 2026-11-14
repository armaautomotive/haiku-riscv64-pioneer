#!/usr/bin/env python3
"""Synthetic split headers exercise bounds; no huge weights or device needed."""
import copy
import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from tenstorrent_flash_next_preflight import FORMATS, GIB, REQUIRED, inspect_shard, plan


def string(value):
    data = value.encode()
    return struct.pack("<Q", len(data)) + data


def encode_value(value):
    if isinstance(value, str):
        return struct.pack("<I", 8) + string(value)
    if isinstance(value, list):
        return struct.pack("<IIQ", 9, 10, len(value)) + struct.pack("<"+"Q"*len(value), *value)
    return struct.pack("<II", 4, value)


def header(metadata, tensors):
    data = bytearray(struct.pack("<4sIQQ", b"GGUF", 3, len(tensors), len(metadata)))
    for key, value in metadata.items():
        data += string(key) + encode_value(value)
    offset = 0
    for name, shape, kind in tensors:
        data += string(name) + struct.pack("<I", len(shape))
        data += struct.pack("<"+"Q"*len(shape), *shape) + struct.pack("<IQ", kind, offset)
        _, block, packed = FORMATS[kind]
        size = packed
        for dim in shape:
            size *= dim
        size //= block
        offset = (offset+size+31)//32*32
    start = (len(data)+31)//32*32
    return bytes(data), start+offset


def model():
    m = {"general.architecture": "qwen4exp", "general.file_type": 15,
         "split.no": 0, "split.count": 2,
         "qwen4exp.ple.head_offsets": [i*20_000_000 for i in range(16)],
         "qwen4exp.ple.head_vocab_sizes": [20_000_000]*16}
    m.update({"qwen4exp."+k: v for k, v in REQUIRED.items()})
    ts = [("token_embd.weight", [2560, 248320], 8),
          ("output.weight", [2560, 248320], 8),
          ("per_layer_token_embd.weight", [160, 320_000_000], 20)]
    for layer in range(48):
        for role in ("gate", "up", "down"):
            ts.append((f"blk.{layer}.ffn_{role}_exps.weight",
                       [640, 2560, 512] if role == "down" else [2560, 640, 512],
                       7 if role == "down" else 12))
    m["split.tensors.count"] = len(ts)
    zero, size_zero = header(m, [])
    one, size_one = header({"split.no": 1, "split.count": 2,
                            "split.tensors.count": len(ts)}, ts)
    return [inspect_shard(zero, size_zero), inspect_shard(one, size_one)], (zero, size_zero, one, size_one)


class PreflightTest(unittest.TestCase):
    def test_inventory(self):
        shards, _ = model()
        p = plan(shards)
        self.assertEqual(p["tensor_count"], 147)
        self.assertEqual(p["ngram_host_bytes"], 28_800_000_000)
        self.assertEqual(p["routed_expert_bytes"], 48*512*(2*2560//256*144*640+640//32*24*2560))
        self.assertLessEqual(p["card_used_bytes"], 21*GIB)
        self.assertTrue(p["host_offloaded_weights_fit"])
        self.assertEqual(p["cold_selected_expert_bytes_per_token"], 10*p["routed_expert_bytes"]//512)
        self.assertEqual(plan(list(reversed(shards))), p)
        full = plan(shards, card_bytes=28*GIB)
        self.assertGreater(full["resident_experts_per_layer"], p["resident_experts_per_layer"])

    def test_split_and_geometry_refusals(self):
        base, _ = model()
        for mutate in (
            lambda s: s.pop(),
            lambda s: s[1]["metadata"].update({"split.no": 0}),
            lambda s: s[1]["metadata"].update({"split.no": 2}),
            lambda s: s[1]["metadata"].update({"split.tensors.count": 1}),
            lambda s: s[0]["metadata"].update({"general.architecture": "qwen35"}),
            lambda s: s[0]["metadata"].update({"general.file_type": 10}),
            lambda s: s[0]["metadata"].update({"qwen4exp.block_count": 49}),
            lambda s: s[1]["metadata"].update({"qwen4exp.embedding_length": 5120}),
            lambda s: s[1]["tensors"][3]["shape"].__setitem__(2, 511),
            lambda s: s[1]["tensors"][3].update({"name": "blk.48.ffn_gate_exps.weight"}),
            lambda s: s[1]["tensors"].append(copy.deepcopy(s[1]["tensors"][0])),
            lambda s: s[0]["metadata"]["qwen4exp.ple.head_offsets"].__setitem__(15, 2**32),
            lambda s: s[0]["metadata"]["qwen4exp.ple.head_offsets"].__setitem__(1, 0),
        ):
            shards = copy.deepcopy(base)
            mutate(shards)
            with self.assertRaises(ValueError):
                plan(shards)
        for params in ({"reserve_bytes": 21*GIB}, {"card_bytes": 29*GIB},
                       {"host_reserve_bytes": 128*GIB}, {"card_bytes": GIB, "reserve_bytes": 0}):
            with self.assertRaises(ValueError):
                plan(base, **params)

    def test_binary_bounds(self):
        _, (zero, zsize, one, osize) = model()
        for data, full in ((zero[:12], zsize), (zero[:-1], zsize),
                           (one, osize-64), (one, 24), (zero, 2**63)):
            with self.assertRaises((ValueError, struct.error)):
                inspect_shard(data, full)
        for meta, ts in (({"general.alignment": 3}, []),
                         ({}, [("x", [640, 2560], 12)]),
                         ({}, [("x", [32], 8), ("x", [32], 8)]),
                         ({"qwen4exp.bad": [0]*129}, [])):
            data, full = header(meta, ts)
            with self.assertRaises(ValueError):
                inspect_shard(data, full)


if __name__ == "__main__":
    unittest.main()
