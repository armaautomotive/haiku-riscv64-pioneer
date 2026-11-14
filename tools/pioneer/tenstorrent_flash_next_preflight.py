#!/usr/bin/env python3
"""Read-only split-GGUF inventory and single-P100A Q4 offload planning.

Accepts local whole shards, or --full-sizes with bounded header captures.
Never opens a device, evaluates templates, allocates weights, or runs a model.
Layout inspection does not authenticate weights or qualify native execution.
"""
import argparse
import collections
import json
import math
import os
import re
import struct

from tenstorrent_gguf_preflight import MAX_HEADER, Reader, FORMATS, SCALARS

FORMATS = dict(FORMATS)
FORMATS.update({6: ("Q5_0", 32, 22), 7: ("Q5_1", 32, 24),
                20: ("IQ4_NL", 32, 18)})
GIB = 1024**3
EXPERT = re.compile(r"blk\.(\d+)\.ffn_(gate|up|down)_exps\.weight\Z")
REQUIRED = {"block_count": 48, "embedding_length": 2560,
            "expert_count": 512, "expert_used_count": 10,
            "expert_feed_forward_length": 640,
            "expert_shared_feed_forward_length": 640,
            "full_attention_interval": 4, "hyper_connection.count": 4,
            "hyper_connection.low_rank": 320}


def metadata_value(reader, kind, keep):
    # Retain small architecture arrays, but never retain tokenizer arrays or
    # templates. Reader already bounds/skips the large tokenizer metadata.
    if kind != 9 or not keep:
        return reader.value(kind, keep)
    element, count = reader.number("I"), reader.number("Q")
    if element not in SCALARS or count > 128:
        raise ValueError("unsupported architecture array")
    values = [reader.number(SCALARS[element]) for _ in range(count)]
    if any(isinstance(v, float) and not math.isfinite(v) for v in values):
        raise ValueError("nonfinite architecture array")
    return values


def inspect_shard(data, full_size):
    if not 24 <= full_size < 2**63 or len(data) > full_size:
        raise ValueError("invalid full shard length")
    r = Reader(data)
    if bytes(r.take(4)) != b"GGUF" or r.number("I") != 3:
        raise ValueError("expected GGUFv3")
    nt, nm = r.number("Q"), r.number("Q")
    if nt > 10000 or nm > 10000:
        raise ValueError("unreasonable directory counts")
    metadata, seen = {}, set()
    for _ in range(nm):
        key = r.string()
        if not key or "\0" in key or len(key) > 512 or key in seen:
            raise ValueError("invalid/duplicate metadata key")
        seen.add(key)
        keep = key.startswith(("qwen4exp.", "split.")) or key in (
            "general.architecture", "general.alignment", "general.file_type")
        value = metadata_value(r, r.number("I"), keep)
        if keep:
            metadata[key] = value
    alignment = metadata.get("general.alignment", 32)
    if type(alignment) is not int or not 1 <= alignment <= 4096 or alignment & (alignment - 1):
        raise ValueError("invalid alignment")
    tensors, seen = [], set()
    for _ in range(nt):
        name, rank = r.string(), r.number("I")
        if not name or "\0" in name or len(name) > 1024 or name in seen or not 1 <= rank <= 4:
            raise ValueError("invalid/duplicate tensor name or rank")
        seen.add(name)
        shape = [r.number("Q") for _ in range(rank)]
        kind, offset = r.number("I"), r.number("Q")
        if any(not 1 <= d < 2**32 for d in shape) or math.prod(shape) >= 2**63:
            raise ValueError("invalid tensor dimensions")
        if kind not in FORMATS:
            raise ValueError(f"unsupported quantization {kind}")
        fmt, block, packed = FORMATS[kind]
        if shape[0] % block or offset % alignment:
            raise ValueError("unaligned packed tensor")
        tensors.append(dict(name=name, shape=shape, type=kind, format=fmt,
                            offset=offset, bytes=math.prod(shape)//block*packed))
    start = (r.offset + alignment - 1)//alignment*alignment
    if start > full_size:
        raise ValueError("header exceeds full shard")
    end = 0
    for t in sorted(tensors, key=lambda t: t["offset"]):
        if t["offset"] < end or start + t["offset"] + t["bytes"] > full_size:
            raise ValueError("overlapping/out-of-bounds tensor")
        t["file_offset"] = start + t["offset"]
        end = t["offset"] + t["bytes"]
    return dict(metadata=metadata, tensors=tensors, full_bytes=full_size,
                header_bytes=r.offset, input_contains_full_file=len(data) == full_size)


def plan(shards, card_bytes=21*GIB, reserve_bytes=2*GIB, host_bytes=128*GIB,
         host_reserve_bytes=16*GIB):
    if not shards or len(shards) > 64:
        raise ValueError("one complete shard set required")
    by_number = {}
    for shard in shards:
        m = shard["metadata"]
        number, count = m.get("split.no", 0), m.get("split.count", 1)
        if type(number) is not int or type(count) is not int or count != len(shards) or number in by_number:
            raise ValueError("incomplete/duplicate split set")
        by_number[number] = shard
    if set(by_number) != set(range(len(shards))):
        raise ValueError("missing split ordinal")
    first = by_number[0]["metadata"]
    if first.get("general.architecture") != "qwen4exp":
        raise ValueError("Flash-Next qwen4exp architecture required")
    if first.get("general.file_type") not in (15, 16):
        raise ValueError("Q4_K_S/M model family required; individual tensors may use other formats")
    for key, expected in REQUIRED.items():
        actual = first.get("qwen4exp." + key)
        if type(actual) is not int or actual != expected:
            raise ValueError(f"unexpected Flash-Next geometry: {key}")
    tensors, names = [], set()
    for number in range(len(shards)):
        shard = by_number[number]
        for key, value in shard["metadata"].items():
            if key.startswith("qwen4exp.") or key.startswith("general."):
                if key in first and first[key] != value:
                    raise ValueError("inconsistent architecture metadata between shards")
        for t in shard["tensors"]:
            if t["name"] in names:
                raise ValueError("duplicate tensor across shards")
            names.add(t["name"])
            tensors.append(dict(t, shard=number))
        if len(shards) > 1 and shard["metadata"].get("split.tensors.count") != sum(len(s["tensors"]) for s in shards):
            raise ValueError("split tensor total mismatch")
    experts, ple = {}, []
    for t in tensors:
        match = EXPERT.fullmatch(t["name"])
        if match:
            layer, role = int(match[1]), match[2]
            expected = [640, 2560, 512] if role == "down" else [2560, 640, 512]
            if layer >= 48 or t["shape"] != expected:
                raise ValueError("invalid routed expert shape")
            experts[layer, role] = t
        elif t["name"] == "per_layer_token_embd.weight":
            ple.append(t)
    if len(experts) != 48*3 or len(ple) != 1 or ple[0]["shape"][0] != 160 or len(ple[0]["shape"]) != 2:
        raise ValueError("complete separate gate/up/down experts and n-gram table required")
    offsets = first.get("qwen4exp.ple.head_offsets")
    vocabs = first.get("qwen4exp.ple.head_vocab_sizes")
    if not isinstance(offsets, list) or not isinstance(vocabs, list) or len(offsets) != 16 or len(vocabs) != 16:
        raise ValueError("16 n-gram head ranges required")
    if any(type(o) is not int or type(v) is not int or o < 0 or v <= 0 or o+v > ple[0]["shape"][1]
           for o, v in zip(offsets, vocabs)):
        raise ValueError("n-gram head range outside table")
    ranges = sorted(zip(offsets, vocabs))
    if any(o+v > next_o for (o, v), (next_o, _) in zip(ranges, ranges[1:])):
        raise ValueError("overlapping n-gram head ranges")
    if "token_embd.weight" not in names or "output.weight" not in names:
        raise ValueError("token/output projections required")
    if not 0 <= reserve_bytes < card_bytes <= 28*GIB or not 0 <= host_reserve_bytes < host_bytes:
        raise ValueError("invalid single-P100A/host budget")
    expert_bytes = sum(t["bytes"] for t in experts.values())
    ple_bytes = ple[0]["bytes"]
    total = sum(t["bytes"] for t in tensors)
    shared = total - expert_bytes - ple_bytes
    available = card_bytes - reserve_bytes - shared
    if available < 0:
        raise ValueError("non-expert device weights plus reserve exceed card budget")
    # Equal expert counts per layer, with an exact byte budget for each layer's
    # mixed formats. Runtime profiling can later choose WHICH experts to pin.
    per_layer = [sum(experts[layer, role]["bytes"]//512 for role in ("gate", "up", "down")) for layer in range(48)]
    each = min(512, available//sum(per_layer))
    resident_expert_bytes = each*sum(per_layer)
    host_offloaded = ple_bytes + expert_bytes - resident_expert_bytes
    return dict(status="OFFLINE_PLAN_ONLY_NOT_ASSISTANT_SUPPORT", architecture="qwen4exp",
                shard_count=len(shards), tensor_count=len(tensors), tensor_bytes=total,
                formats=dict(collections.Counter(t["format"] for t in tensors)),
                ngram_host_bytes=ple_bytes, routed_expert_bytes=expert_bytes,
                shared_device_weight_bytes=shared, card_budget_bytes=card_bytes,
                card_runtime_reserve_bytes=reserve_bytes, resident_experts_per_layer=each,
                resident_expert_bytes=resident_expert_bytes,
                card_used_bytes=shared+resident_expert_bytes+reserve_bytes,
                host_offloaded_weight_bytes=host_offloaded,
                host_weight_budget_bytes=host_bytes-host_reserve_bytes,
                host_offloaded_weights_fit=host_offloaded <= host_bytes-host_reserve_bytes,
                host_full_source_mirror_fits=total <= host_bytes-host_reserve_bytes,
                cold_selected_expert_bytes_per_token=10*sum(per_layer),
                # Bytes touched, not a PCIe or token-rate prediction. Host
                # execution exchanges activations rather than these weights.
                full_weight_authentication_required=True,
                native_transport_and_graph_required=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("shards", nargs="+")
    parser.add_argument("--full-sizes", nargs="+", type=int,
                        help="full lengths for header captures, in input order")
    parser.add_argument("--card-reserve-gib", type=int, default=2)
    parser.add_argument("--card-gib", type=int, default=21,
                        help="qualified bank capacity, default 7 x 3 GiB; 28 is a hypothetical full-card plan")
    parser.add_argument("--host-reserve-gib", type=int, default=16)
    args = parser.parse_args()
    try:
        sizes = args.full_sizes or [os.stat(path).st_size for path in args.shards]
        if len(sizes) != len(args.shards):
            raise ValueError("one full length required per input")
        shards = []
        for path, size in zip(args.shards, sizes):
            with open(path, "rb") as source:
                shards.append(inspect_shard(source.read(MAX_HEADER), size))
        print(json.dumps(plan(shards, card_bytes=args.card_gib*GIB, reserve_bytes=args.card_reserve_gib*GIB,
                              host_reserve_bytes=args.host_reserve_gib*GIB), indent=2))
    except (OSError, ValueError, UnicodeError, struct.error) as error:
        parser.exit(1, f"STOP: {error}; no device/model execution\n")


if __name__ == "__main__":
    main()
