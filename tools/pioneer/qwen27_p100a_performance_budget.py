#!/usr/bin/env python3
"""Read-only, conditional performance budget; never opens an accelerator.

Uses the actual GGUF layout, a host operation inventory of the production
templates, and an existing closed memory-test receipt. Derived rates are
NOT model benchmarks or promises. No weights or model text are emitted.
"""
import argparse
import collections
from decimal import Decimal, localcontext
import hashlib
import json
import os
import pathlib
import re
import sys

from tenstorrent_gguf_preflight import FORMATS, MAX_HEADER, inspect

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent / "tests"))
from compare_pioneer_dram_cohort_log import check as check_cohort

MODEL_BYTES = 18973870528
MODEL_SHA = "c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
FORMAT_BY_NAME = {value[0]: value[1:] for value in FORMATS.values()}
FIELDS = ("ops", "configs", "drains", "matrix")


def inventory(body):
    lines = body.splitlines()
    marker = "HOST_TEMPLATE_OPERATION_COUNTS_ONLY;NO_DEVICE_NO_CYCLE_OR_TPS_MEASUREMENT"
    if len(lines) != 20 or lines[-1] != marker:
        raise ValueError("incomplete host operation inventory")
    result = {}
    for line in lines[:-1]:
        match = re.fullmatch(r"(BEGIN|METADATA|GROUP) buffer([01]) group([0-7]) "
                             r"ops([0-9]+) configs([0-9]+) drains([0-9]+) matrix([0-9]+)", line)
        if not match:
            raise ValueError("invalid host inventory record")
        scope, buffer, group, *values = match.groups()
        key = (scope, int(buffer), int(group))
        counts = dict(zip(FIELDS, map(int, values)))
        if key in result or not 0 < counts["ops"] < 1000000 or counts["matrix"] > counts["ops"]:
            raise ValueError("duplicate or invalid operation counts")
        result[key] = counts
    expected = {("BEGIN", 0, 0)} | {("METADATA", b, 0) for b in range(2)}
    expected |= {("GROUP", b, g) for b in range(2) for g in range(8)}
    if set(result) != expected:
        raise ValueError("missing inventory scope")
    for scope, _, group in expected:
        if scope != "BEGIN" and result[(scope, 0, group)] != result[(scope, 1, group)]:
            raise ValueError("buffer operation counts differ")
    block = {field: result[("METADATA", 0, 0)][field]
             + sum(result[("GROUP", 0, group)][field] for group in range(8)) for field in FIELDS}
    return result[("BEGIN", 0, 0)], block


def layout_budget(tensors):
    """One-pass storage/traffic MODEL, not a count of actual DRAM requests."""
    by_format = collections.defaultdict(int)
    counts = collections.Counter()
    pairs = blocks = dense_ops = float_bytes = 0
    names = set()
    for tensor in tensors:
        name, shape, fmt = tensor["name"], tensor["shape"], tensor["format"]
        if name in names:
            raise ValueError("duplicate tensor")
        names.add(name)
        if name == "token_embd.weight":
            if shape != [5120, 248320] or fmt != "Q4_K":
                raise ValueError("unexpected embedding")
            continue  # Lookup does not stream the entire embedding table.
        if fmt == "F32":
            float_bytes += tensor["bytes"]
            continue
        if fmt not in ("Q4_K", "Q6_K", "Q8_0") or len(shape) != 2:
            raise ValueError("unsupported matrix format")
        width, rows = shape
        block, block_bytes = FORMAT_BY_NAME[fmt]
        if width % block or rows % 16:
            raise ValueError("non-tile matrix")
        tile_bytes = (16 * block_bytes + 63) // 64 * 64
        by_format[fmt] += width // block * rows // 16 * tile_bytes
        counts[fmt] += 1
        dense_ops += 2 * width * rows
        if fmt == "Q4_K":
            if not re.fullmatch(r"blk\.[0-9]+\.ffn_(down|gate|up)\.weight", name):
                raise ValueError("unrecognized active Q4 matrix")
            if (width, rows) not in ((5120, 17408), (17408, 5120)):
                raise ValueError("unexpected Q4 shape")
            pairs += rows // 32
            blocks += rows // 32 * (width // 256)
    if "token_embd.weight" not in names or "output.weight" not in names:
        raise ValueError("missing global matrix")
    return {"tiled_quantized_bytes_by_format": dict(sorted(by_format.items())),
            "active_quantized_matrix_counts": dict(sorted(counts.items())),
            "non_embedding_f32_bytes": float_bytes,
            "one_pass_weight_bytes_excluding_embedding": sum(by_format.values()) + float_bytes,
            "quantized_dense_multiply_add_flops": dense_ops,
            "q4_output_pairs": pairs, "q4_pair_k256_blocks": blocks}


def calculate(tensors, issue_body, cohort_body):
    begin, per_block = inventory(issue_body)
    memory = check_cohort(cohort_body, 15, 256, 16, direct_tlb=True)
    if memory["timingState"] != 1:
        raise ValueError("no accepted memory observation bracket")
    layout = layout_budget(tensors)
    # Exact widths/counts of the fixed model used by the completed trials.
    if layout["active_quantized_matrix_counts"] != {"Q4_K": 192, "Q6_K": 17, "Q8_0": 288}:
        raise ValueError("not the fixed 27B matrix inventory")
    q4 = {field: begin[field] * layout["q4_output_pairs"]
          + per_block[field] * layout["q4_pair_k256_blocks"] for field in FIELDS}
    with localcontext() as context:
        context.prec = 30
        weight_bytes = Decimal(layout["one_pass_weight_bytes_excluding_embedding"])
        target = Decimal(20)
        required = weight_bytes * target / Decimal(10**9)
        rated = Decimal(448)
        # 119 workers, one T0 instruction stream per tile. Excludes ALL
        # non-Q4 work, extra Drain-issued instructions, Config/NoC/loop work,
        # wait states, imbalance and controller synchronization.
        issue_ceiling = Decimal(119) * Decimal(1350000000) / Decimal(q4["ops"])
        return {"scope": "conditional analytical budgets;NOT measured model speed or universal card limits",
                "model_bytes": MODEL_BYTES, "model_sha256": MODEL_SHA,
                "layout": layout, "q4_template_begin": begin, "q4_template_k256_block": per_block,
                "q4_template_operations_per_token": q4,
                "q4_explicit_read_requests_per_token": layout["q4_pair_k256_blocks"] * 26,
                "q4_weight_read_bytes_per_token": layout["q4_pair_k256_blocks"] * 4608,
                "q4_root_l1_input_read_bytes_per_token": layout["q4_pair_k256_blocks"] * 512,
                "rated_memory_GBps_not_measured": str(rated),
                "existing_memory_test_observed_GBps": memory["bandwidthGBps"],
                "required_one_pass_weight_GBps_at_20_tps": str(required),
                "required_percent_of_rated_bandwidth": str(required / rated * 100),
                "conditional_weight_only_tps_at_rated_bandwidth": str(rated * 10**9 / weight_bytes),
                "conditional_weight_only_tps_at_test_bandwidth":
                    [str(Decimal(rate) * 10**9 / weight_bytes) for rate in memory["bandwidthGBps"]],
                "conditional_q4_only_issue_ceiling_tps_at_nominal_1_35GHz": str(issue_ceiling),
                "limitations": ["Weight budget assumes one pass over tiled non-embedding weights;not actual traffic telemetry.",
                                "Embedding lookup,activation/state traffic,extra reads and all computation add costs.",
                                "Memory test repeated fixed 8MiB windows;not model access geometry or peak bandwidth.",
                                "Operation inventory runs host templates;does not execute or time the card.",
                                "Issue budget assumes documented one T0 dequeue/cycle at nominal clock;not clock calibration.",
                                "Other algorithms,formats,parallel streams or reuse can change these conditional budgets."]}


def read_model(path):
    with path.open("rb") as source:
        before = os.fstat(source.fileno())
        if before.st_size != MODEL_BYTES:
            raise ValueError("unexpected model length")
        header = source.read(MAX_HEADER)
        digest = hashlib.sha256(header)
        while True:
            chunk = source.read(8 * 1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
        after = os.fstat(source.fileno())
        identity = lambda value: (value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns, value.st_ctime_ns)
        if identity(before) != identity(after) or identity(before) != identity(path.stat()) or digest.hexdigest() != MODEL_SHA:
            raise ValueError("model changed or SHA-256 mismatch")
    parsed = inspect(header, MODEL_BYTES, include_tensors=True)
    if parsed["tensor_count"] != 851 or parsed["metadata"].get("general.architecture") != "qwen35":
        raise ValueError("unexpected model architecture")
    return parsed["tensors"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=pathlib.Path)
    parser.add_argument("--issue-inventory", type=pathlib.Path, required=True)
    parser.add_argument("--closed-cohort-log", type=pathlib.Path, required=True)
    args = parser.parse_args()
    if not __debug__:
        parser.exit(2, "Assertions required by the closed-receipt reader\n")
    try:
        result = calculate(read_model(args.model), args.issue_inventory.read_text(), args.closed_cohort_log.read_text())
    except (ValueError, AssertionError, OSError, UnicodeError) as error:
        parser.exit(1, f"STOP: {error}; no accelerator accessed\n")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
