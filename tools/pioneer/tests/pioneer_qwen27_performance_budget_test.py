#!/usr/bin/env python3
"""Host tests only: count real templates; check budget arithmetic/refusals."""
import copy
import json
from decimal import Decimal
import pathlib
import subprocess
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
TOOLS = HERE.parent
sys.path.insert(0, str(TOOLS))
import qwen27_p100a_performance_budget as budget


def model_shapes():
    """Synthetic layout-only fixture; no original weights or predictions."""
    tensors = []

    def add(name, width, rows, fmt):
        block, block_bytes = budget.FORMAT_BY_NAME[fmt]
        tensors.append({"name": name, "shape": [width, rows], "format": fmt,
                        "bytes": width * rows // block * block_bytes})

    add("token_embd.weight", 5120, 248320, "Q4_K")
    add("output.weight", 5120, 248320, "Q6_K")
    for layer in range(64):
        add(f"blk.{layer}.ffn_down.weight", 17408, 5120, "Q4_K")
        for suffix in ("gate", "up"):
            add(f"blk.{layer}.ffn_{suffix}.weight", 5120, 17408, "Q4_K")
        if layer % 4 != 3:
            for suffix, width, rows in (("attn_gate", 5120, 6144), ("attn_qkv", 5120, 10240),
                                         ("ssm_out", 6144, 5120), ("ssm_alpha", 5120, 48),
                                         ("ssm_beta", 5120, 48)):
                add(f"blk.{layer}.{suffix}.weight", width, rows, "Q8_0")
        else:
            add(f"blk.{layer}.attn_output.weight", 6144, 5120, "Q6_K")
            for suffix, rows in (("attn_k", 1024), ("attn_v", 1024), ("attn_q", 12288)):
                add(f"blk.{layer}.{suffix}.weight", 5120, rows, "Q8_0")
    tensors.append({"name": "synthetic_float_total", "shape": [2645504],
                    "format": "F32", "bytes": 10582016})
    return tensors


class BudgetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="pioneer-issue-budget-")
        binary = pathlib.Path(cls.directory.name) / "inventory"
        subprocess.run(["c++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(TOOLS / "pioneer_q38_bf16_issue_inventory.cpp"),
                        "-o", str(binary)], check=True)
        result = subprocess.run([str(binary)], text=True, capture_output=True, check=True)
        if result.stderr:
            raise AssertionError(result.stderr)
        cls.issue = result.stdout
        cls.cohort = (TOOLS / "benchmarks/p100a-dram-cohort-direct-20261103/depth16.log").read_text()

    @classmethod
    def tearDownClass(cls):
        cls.directory.cleanup()

    def test_actual_template_counts(self):
        begin, block = budget.inventory(self.issue)
        self.assertEqual(begin, {"ops": 94, "configs": 69, "drains": 20, "matrix": 0})
        self.assertEqual(block, {"ops": 4664, "configs": 182, "drains": 98, "matrix": 128})

    def test_complete_fixture_budget(self):
        result = budget.calculate(model_shapes(), self.issue, self.cohort)
        layout = result["layout"]
        self.assertEqual(layout["q4_output_pairs"], 79872)
        self.assertEqual(layout["q4_pair_k256_blocks"], 2088960)
        self.assertEqual(layout["one_pass_weight_bytes_excluding_embedding"], 18682484736)
        self.assertEqual(result["q4_weight_read_bytes_per_token"], 9625927680)
        self.assertEqual(result["q4_explicit_read_requests_per_token"], 54312960)
        self.assertEqual(result["q4_template_operations_per_token"]["ops"], 9750417408)
        self.assertLess(Decimal(result["conditional_q4_only_issue_ceiling_tps_at_nominal_1_35GHz"]), 20)
        self.assertEqual(result["required_one_pass_weight_GBps_at_20_tps"], "373.64969472")

    def test_inventory_refusals(self):
        for body in (self.issue[:-100], self.issue + "EXTRA\n", self.issue.replace("GROUP buffer0 group0", "GROUP buffer0 group1"),
                     self.issue.replace("ops507", "ops0", 1), self.issue.replace("matrix16", "matrix999", 1),
                     self.issue.replace("GROUP buffer1 group7 ops507", "GROUP buffer1 group7 ops508")):
            with self.subTest(body=body[-80:]), self.assertRaises(ValueError):
                budget.inventory(body)

    def test_saved_result_matches_recalculation(self):
        evidence = TOOLS / "benchmarks/qwen38-performance-budget-20261106"
        self.assertEqual(self.issue, (evidence / "issue-inventory.log").read_text())
        self.assertEqual(budget.calculate(model_shapes(), self.issue, self.cohort),
                         json.loads((evidence / "budget.json").read_text()))

    def test_layout_refusals(self):
        fixture = model_shapes()
        bad = copy.deepcopy(fixture)
        bad[2]["shape"][0] = 17409
        wrong_format = copy.deepcopy(fixture)
        wrong_format[2]["format"] = "BF16"
        wrong_q4 = copy.deepcopy(fixture)
        wrong_q4[2]["name"] = "unexpected.weight"
        for tensors in (fixture[1:], fixture + fixture[:1], bad, wrong_format, wrong_q4):
            with self.subTest(count=len(tensors)), self.assertRaises(ValueError):
                budget.layout_budget(tensors)
        with self.assertRaises(ValueError):
            budget.calculate(fixture[:-2], self.issue, self.cohort)

    def test_invalid_closed_memory_receipt_refused(self):
        for body in (self.cohort[:-100], self.cohort.replace("retained0", "retained1"),
                     self.cohort.replace("elapsed_us50470", "elapsed_us50471")):
            with self.assertRaises((AssertionError, ValueError)):
                budget.calculate(model_shapes(), self.issue, body)

    def test_wrong_model_length_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "not-model"
            path.touch()
            with self.assertRaises(ValueError):
                budget.read_model(path)


if __name__ == "__main__":
    unittest.main()
