#!/usr/bin/env python3
"""Bounded, read-only GGUF v3 metadata inspection. Never executes a model.

Accepts a header range plus declared FULL length; absent tensor bytes are not
treated as zeros or as a runnable model. No chat-template evaluation, pickle,
network, device access or full-weight dequantization. GGUF format reference:
https://github.com/ggml-org/ggml/blob/master/docs/gguf.md
"""
import argparse
import collections
import json
import math
import struct

MAX_HEADER = 32 * 1024 * 1024
SCALARS = {0: "B", 1: "b", 2: "H", 3: "h", 4: "I", 5: "i", 6: "f", 7: "B", 10: "Q", 11: "q", 12: "d"}
# GGML enum -> (name, elements per block, bytes per block).
FORMATS = {0: ("F32", 1, 4), 1: ("F16", 1, 2), 2: ("Q4_0", 32, 18),
           3: ("Q4_1", 32, 20), 8: ("Q8_0", 32, 34), 10: ("Q2_K", 256, 84),
           11: ("Q3_K", 256, 110), 12: ("Q4_K", 256, 144),
           13: ("Q5_K", 256, 176), 14: ("Q6_K", 256, 210), 15: ("Q8_K", 256, 292),
           30: ("BF16", 1, 2)}


class Reader:
    def __init__(self, data):
        if len(data) > MAX_HEADER:
            raise ValueError("header exceeds 32 MiB limit")
        self.data, self.offset = memoryview(data), 0

    def take(self, count):
        if count < 0 or count > len(self.data) - self.offset:
            raise ValueError(f"truncated header at byte {self.offset}; need {count} bytes")
        start = self.offset
        self.offset += count
        return self.data[start:self.offset]

    def number(self, fmt):
        return struct.unpack("<" + fmt, self.take(struct.calcsize("<" + fmt)))[0]

    def string(self, decode=True):
        length = self.number("Q")
        if length > MAX_HEADER:
            raise ValueError("oversized string")
        raw = self.take(length)
        return bytes(raw).decode("utf-8") if decode else None

    def value(self, kind, keep=False):
        if kind in SCALARS:
            value = self.number(SCALARS[kind])
            if kind == 7 and value not in (0, 1):
                raise ValueError("invalid GGUF bool")
            if isinstance(value, float) and not math.isfinite(value):
                raise ValueError("nonfinite metadata scalar")
            return value if keep else None
        if kind == 8:
            return self.string(keep)
        if kind != 9:
            raise ValueError(f"unsupported metadata type {kind}")
        element, count = self.number("I"), self.number("Q")
        if count > 1_000_000 or element == 9:
            raise ValueError("oversized or nested metadata array")
        if element in SCALARS:
            self.take(count * struct.calcsize("<" + SCALARS[element]))
        elif element == 8:
            for _ in range(count):
                self.string(False)
        else:
            raise ValueError("unsupported array element type")
        return {"array_type": element, "count": count} if keep else None


def inspect(data, full_size):
    if not 24 <= full_size < 2**63:
        raise ValueError("invalid declared full size")
    r = Reader(data)
    if bytes(r.take(4)) != b"GGUF" or r.number("I") != 3:
        raise ValueError("expected GGUF version 3")
    tensor_count, metadata_count = r.number("Q"), r.number("Q")
    if tensor_count > 100_000 or metadata_count > 100_000:
        raise ValueError("unreasonable header counts")
    metadata = {}
    seen = set()
    for _ in range(metadata_count):
        key = r.string()
        if len(key) > 512 or key in seen:
            raise ValueError("oversized/duplicate metadata key")
        seen.add(key)
        keep = key in ("general.architecture", "general.alignment", "general.name", "general.file_type") or key.startswith(("qwen3.", "qwen35."))
        value = r.value(r.number("I"), keep)
        if keep:
            metadata[key] = value
    alignment = metadata.get("general.alignment", 32)
    if not isinstance(alignment, int) or not 1 <= alignment <= 4096 or alignment & (alignment-1):
        raise ValueError("invalid tensor alignment")
    tensors, names = [], set()
    for _ in range(tensor_count):
        name, rank = r.string(), r.number("I")
        if len(name) > 1024 or name in names or not 1 <= rank <= 4:
            raise ValueError("invalid tensor name/rank")
        names.add(name)
        shape = [r.number("Q") for _ in range(rank)]
        if any(not 1 <= dim < 2**32 for dim in shape) or math.prod(shape) >= 2**63:
            raise ValueError("invalid tensor dimensions")
        kind, offset = r.number("I"), r.number("Q")
        if kind not in FORMATS:
            raise ValueError(f"unsupported tensor quantization type {kind}")
        format_name, block, block_bytes = FORMATS[kind]
        if shape[0] % block or offset % alignment:
            raise ValueError("misaligned tensor/block")
        size = math.prod(shape) // block * block_bytes
        tensors.append({"name": name, "shape": shape, "format": format_name,
                        "offset": offset, "bytes": size})
    data_start = (r.offset+alignment-1)//alignment*alignment
    end = 0
    for tensor in sorted(tensors, key=lambda item: item["offset"]):
        if tensor["offset"] < end or data_start+tensor["offset"]+tensor["bytes"] > full_size:
            raise ValueError("overlapping tensor or bounds beyond declared full file")
        end = tensor["offset"]+tensor["bytes"]
        tensor["file_offset"] = data_start+tensor["offset"]
    return {"metadata": metadata, "tensor_count": tensor_count,
            "header_bytes": r.offset, "data_start": data_start,
            "declared_full_bytes": full_size, "tensor_bytes": sum(t["bytes"] for t in tensors),
            "formats": dict(collections.Counter(t["format"] for t in tensors)),
            "matrix_shapes": dict(collections.Counter(
                f'{t["format"]}:{t["shape"][0]}x{t["shape"][1]}' for t in tensors if len(t["shape"]) == 2)),
            # Layout inventory only: no claim that the shapes, signed values,
            # architecture, arena or full model are executable on the card.
            "q8_packed_matrix_bytes": sum(math.prod(t["shape"]) // 32 * 36
                                          for t in tensors if t["format"] == "Q8_0" and len(t["shape"]) == 2),
            "selected_tensors": [t for t in tensors if t["name"].startswith(("blk.0.", "blk.3."))],
            "header_only_not_runnable": len(data) < full_size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("header")
    parser.add_argument("--full-size", type=int, required=True)
    args = parser.parse_args()
    with open(args.header, "rb") as source:
        data = source.read(MAX_HEADER+1)
    try:
        print(json.dumps(inspect(data, args.full_size), indent=2))
    except (ValueError, UnicodeError, struct.error) as error:
        parser.exit(1, f"STOP: {error}; no model/device was executed\n")


if __name__ == "__main__":
    main()
