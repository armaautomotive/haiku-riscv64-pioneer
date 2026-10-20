#!/usr/bin/env python3
"""Extract the BFS filesystem from an EFI/BFS MBR image using its actual geometry."""
import argparse
import os
from pathlib import Path
import stat
import struct
import tempfile


def geometry(image):
    with image.open("rb") as stream:
        mbr = stream.read(512)
        if len(mbr) != 512 or mbr[510:] != b"\x55\xaa":
            raise ValueError("Missing MBR signature")
        partitions = [mbr[446 + 16*i:462 + 16*i] for i in range(4)]
        bfs = [entry for entry in partitions if entry[4] == 0xeb]
        if len(bfs) != 1:
            raise ValueError("Expected exactly one BFS partition")
        start, sectors = struct.unpack_from("<II", bfs[0], 8)
        offset, partition_size = start * 512, sectors * 512
        if start == 0 or sectors == 0 or offset + partition_size > image.stat().st_size:
            raise ValueError("BFS partition exceeds image boundaries")
        for entry in partitions:
            if entry[4] and entry is not bfs[0]:
                other_start, other_size = struct.unpack_from("<II", entry, 8)
                if max(start, other_start) < min(start + sectors, other_start + other_size):
                    raise ValueError("Overlapping image partitions")
        stream.seek(offset + 512)
        header = stream.read(116)
        if len(header) != 116:
            raise ValueError("Truncated BFS superblock")
        u32 = lambda position: struct.unpack_from("<I", header, position)[0]
        if (u32(32), u32(36), u32(68), u32(112)) != (
                0x42465331, 0x42494745, 0xdd121031, 0x15b6830e):
            raise ValueError("Not a supported little-endian BFS filesystem")
        block_size, shift = u32(40), u32(44)
        blocks, used = struct.unpack_from("<qq", header, 48)
        if (block_size not in (1024, 2048, 4096, 8192) or shift not in (10, 11, 12, 13)
                or 1 << shift != block_size or not 0 <= used <= blocks or blocks <= 0):
            raise ValueError("Invalid BFS block geometry")
        size = blocks * block_size
        if size > partition_size or size % (1024 * 1024):
            raise ValueError("Expected a whole-MiB BFS filesystem fitting the partition")
        return offset, size


def extract(image, output):
    if not image.is_file() or image.is_symlink() or image.resolve() == output.resolve():
        raise ValueError("Source must be a regular image distinct from the output")
    if output.is_symlink() or (output.exists() and not stat.S_ISREG(output.stat().st_mode)):
        raise ValueError("Output must be a regular file")
    offset, size = geometry(image)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output.parent, prefix=".bfs-", delete=False) as target:
            temporary = Path(target.name)
            with image.open("rb") as source:
                source.seek(offset)
                remaining = size
                while remaining:
                    block = source.read(min(remaining, 1024 * 1024))
                    if not block:
                        raise ValueError("Truncated BFS filesystem")
                    target.write(block)
                    remaining -= len(block)
            target.flush()
            os.fsync(target.fileno())
        os.replace(temporary, output)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return size


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(f"Extracted {extract(args.image, args.output)} BFS bytes to {args.output}")
