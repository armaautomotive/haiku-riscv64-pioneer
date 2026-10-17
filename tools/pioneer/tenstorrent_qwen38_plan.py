#!/usr/bin/env python3
"""OFFLINE Qwen3.8-27B packed seven-bank residency plan, never device access.

All addresses are PROPOSED data extents, NOT allocation/route authority. The
installed driver still permits only its qualified bank-zero 2GiB arena.
No model/template execution, pickle, network, weight expansion, or GPU access.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import stat

from tenstorrent_gguf_preflight import inspect, MAX_HEADER

FULL_BYTES = 18_973_870_528
SHA256 = "c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
REVISION = "71bc7b627595dc8a91039addd9c791ae548d6747"
PAGE = 4096
FORMATS = {"F32": (1, 4), "Q4_K": (256, 144), "Q6_K": (256, 210), "Q8_0": (32, 34)}


def expected_tensors():
    expected = {"token_embd.weight": ([5120, 248320], "Q4_K"),
                "output.weight": ([5120, 248320], "Q6_K"),
                "output_norm.weight": ([5120], "F32")}
    for layer in range(64):
        common = {"attn_norm.weight": ([5120], "F32"),
                  "post_attention_norm.weight": ([5120], "F32"),
                  "ffn_gate.weight": ([5120, 17408], "Q4_K"),
                  "ffn_up.weight": ([5120, 17408], "Q4_K"),
                  "ffn_down.weight": ([17408, 5120], "Q4_K")}
        if layer % 4 == 3:
            common.update({"attn_q.weight": ([5120, 12288], "Q8_0"),
                           "attn_k.weight": ([5120, 1024], "Q8_0"),
                           "attn_v.weight": ([5120, 1024], "Q8_0"),
                           "attn_q_norm.weight": ([256], "F32"),
                           "attn_k_norm.weight": ([256], "F32"),
                           "attn_output.weight": ([6144, 5120], "Q6_K")})
        else:
            common.update({"attn_gate.weight": ([5120, 6144], "Q8_0"),
                           "attn_qkv.weight": ([5120, 10240], "Q8_0"),
                           "ssm_a": ([48], "F32"), "ssm_dt.bias": ([48], "F32"),
                           "ssm_norm.weight": ([128], "F32"),
                           "ssm_alpha.weight": ([5120, 48], "Q8_0"),
                           "ssm_beta.weight": ([5120, 48], "Q8_0"),
                           "ssm_conv1d.weight": ([4, 10240], "F32"),
                           "ssm_out.weight": ([6144, 5120], "Q8_0")})
        expected.update({f"blk.{layer}.{name}": value for name, value in common.items()})
    return expected


def validate_inventory(inventory):
    metadata = inventory["metadata"]
    required = {"general.architecture": "qwen35", "qwen35.block_count": 64,
                "qwen35.embedding_length": 5120, "qwen35.feed_forward_length": 17408,
                "qwen35.attention.head_count": 24, "qwen35.attention.head_count_kv": 4,
                "qwen35.attention.key_length": 256, "qwen35.attention.value_length": 256,
                "qwen35.ssm.conv_kernel": 4, "qwen35.ssm.state_size": 128,
                "qwen35.ssm.group_count": 16, "qwen35.ssm.time_step_rank": 48,
                "qwen35.ssm.inner_size": 6144, "qwen35.full_attention_interval": 4,
                "qwen35.rope.dimension_count": 64}
    if any(metadata.get(key) != value for key, value in required.items()):
        raise ValueError("not the pinned Qwen3.8-27B hybrid geometry")
    expected = expected_tensors()
    tensors = inventory["tensors"]
    if len(tensors) != len(expected) or len({t["name"] for t in tensors}) != len(expected):
        raise ValueError("missing/duplicate/unexpected tensors")
    end = 0
    for tensor in sorted(tensors, key=lambda t: t["file_offset"]):
        shape, kind = expected.get(tensor["name"], (None, None))
        if tensor["shape"] != shape or tensor["format"] != kind:
            raise ValueError(f"unexpected tensor geometry/format: {tensor['name']}")
        block, block_bytes = FORMATS[kind]
        elements = shape[0] * (shape[1] if len(shape) == 2 else 1)
        size = elements // block * block_bytes
        start = tensor["file_offset"]
        if shape[0] % block or tensor["bytes"] != size or start < end or start < inventory["data_start"] or start + size > FULL_BYTES:
            raise ValueError("tensor block/overlap/full-file bounds failure")
        end = start + size


class ProposedBanks:
    def __init__(self, count=7, capacity=3 * 1024**3):
        # Conservative OFFLINE budget,NOT permission to write these extents.
        self.base = 0x04000000
        if count != 7 or not 0 < capacity <= 0xFF000000 - self.base or capacity % PAGE:
            raise ValueError("requires seven bounded page-aligned proposed arenas")
        self.capacity, self.used, self.fragments = capacity, [0] * count, []

    def allocate(self, bank, name, size, kind, **extra):
        allocated = (size + PAGE - 1) // PAGE * PAGE
        if size <= 0 or not 0 <= bank < 7 or allocated > self.capacity - self.used[bank]:
            raise ValueError(f"proposed bank budget exhausted: {name}")
        fragment = dict(bank_index=bank, bank_address=self.base + self.used[bank],
                        name=name, kind=kind, bytes=size, allocated_bytes=allocated, **extra)
        self.used[bank] += allocated
        self.fragments.append(fragment)

    def stripe(self, name, rows, row_bytes, unit, kind, source=None, format_name=None):
        if rows <= 0 or row_bytes <= 0 or unit <= 0:
            raise ValueError("invalid proposed row partition")
        chunks, tail = divmod(rows, unit)
        if chunks < 7:
            counts = [rows]
        else:
            counts = [(chunks // 7 + (i < chunks % 7)) * unit for i in range(7)]
            counts[-1] += tail
        banks = sorted(range(7), key=lambda bank: (self.used[bank], bank))
        first = 0
        for bank, count in zip(banks, counts):
            self.allocate(bank, name, count * row_bytes, kind, first_row=first, rows=count,
                          total_rows=rows, row_bytes=row_bytes,
                          source_offset=None if source is None else source + first * row_bytes,
                          format=format_name, zero_initialization_required=source is None)
            first += count
        if first != rows:
            raise ValueError("incomplete proposed row partition")


def plan(inventory, context=8192, capacity=3 * 1024**3, workspace_per_bank=64 * 1024**2):
    if not isinstance(context, int) or not 1 <= context <= 262144:
        raise ValueError("context outside pinned model bounds")
    if not isinstance(workspace_per_bank, int) or workspace_per_bank < 0 or workspace_per_bank % PAGE:
        raise ValueError("workspace must be a nonnegative page-aligned budget")
    validate_inventory(inventory)
    banks = ProposedBanks(capacity=capacity)
    for tensor in inventory["tensors"]:
        shape, kind = tensor["shape"], tensor["format"]
        block, block_bytes = FORMATS[kind]
        rows = shape[1] if len(shape) == 2 else 1
        banks.stripe(tensor["name"], rows, shape[0] // block * block_bytes, 64, "packed_weight",
                     tensor["file_offset"], kind)
    state_bytes = dict(delta_fp32=0, convolution_fp32=0, kv_fp16=0)
    for layer in range(64):
        if layer % 4 == 3:
            # Eight independent K/V heads,each256 values/position at FP16.
            banks.stripe(f"state.{layer}.kv_fp16", 8, context * 256 * 2, 1, "kv_fp16")
            state_bytes["kv_fp16"] += 8 * context * 256 * 2
        else:
            banks.stripe(f"state.{layer}.delta_fp32", 48, 128 * 128 * 4, 1, "delta_fp32")
            banks.stripe(f"state.{layer}.convolution_fp32", 10240, 3 * 4, 64, "convolution_fp32")
            state_bytes["delta_fp32"] += 48 * 128 * 128 * 4
            state_bytes["convolution_fp32"] += 10240 * 3 * 4
    if workspace_per_bank:
        for bank in range(7):
            banks.allocate(bank, "proposed_workspace", workspace_per_bank, "workspace",
                           source_offset=None, zero_initialization_required=True)
    return dict(schema="pioneer.qwen38.offline-residency.v1", device_access=False,
                executable_by_installed_backend=False, route_or_allocation_authority=False,
                model_execution_tested=False, source_revision=REVISION, expected_full_sha256=SHA256,
                context=context, layers=64, delta_layers=48, full_attention_layers=16,
                tensors=len(inventory["tensors"]), packed_weight_bytes=inventory["tensor_bytes"],
                state_bytes=state_bytes, workspace_per_bank=workspace_per_bank,
                proposed_bank_base=banks.base, proposed_capacity_per_bank=capacity,
                allocated_bytes_per_bank=banks.used, allocated_bytes=sum(banks.used),
                fragments=banks.fragments,
                required_unimplemented=["guarded multi-bank allocation/immutable route leases",
                    "on-card mixed Q4_K/Q6_K/Q8_0 kernels without full expansion",
                    "Gated DeltaNet/convolution/gated attention graph execution",
                    "full-model numerical validation and sustained native benchmarks"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--header-only", action="store_true", help="accept previously pinned header range,not a full model")
    parser.add_argument("--verify-full", action="store_true", help="require and SHA256-verify the full pinned model")
    parser.add_argument("--context", type=int, default=8192)
    parser.add_argument("--output", type=Path, help="fresh offline JSON manifest,never a device/driver input")
    args = parser.parse_args()
    if args.header_only and args.verify_full:
        parser.error("header-only data cannot verify the full model")
    try:
        with args.source.open("rb") as source:
            info = os.fstat(source.fileno())
            if not stat.S_ISREG(info.st_mode) or (not args.header_only and info.st_size != FULL_BYTES):
                raise ValueError("expected regular complete pinned model or explicit header-only range")
            prefix = source.read(MAX_HEADER)
            digest = None
            if args.verify_full:
                source.seek(0)
                sha = hashlib.sha256()
                while True:
                    chunk = source.read(4 * 1024**2)
                    if not chunk:
                        break
                    sha.update(chunk)
                digest = sha.hexdigest()
                after = os.fstat(source.fileno())
                if digest != SHA256 or (info.st_size, info.st_mtime_ns, info.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
                    raise ValueError("full model SHA256 mismatch or file changed during verification")
        inventory = inspect(prefix, FULL_BYTES, include_tensors=True)
        result = plan(inventory, args.context)
        result.update(complete_file_present=not args.header_only, full_sha256_verified=digest is not None)
        if args.output:
            with args.output.open("x", encoding="utf-8") as target:
                json.dump(result, target, indent=2)
                target.write("\n")
        print(json.dumps({key: value for key, value in result.items() if key != "fragments"}, indent=2))
    except (ValueError, OSError, KeyError) as error:
        parser.exit(1, f"STOP: {error}; no model or device was executed\n")


if __name__ == "__main__":
    main()
