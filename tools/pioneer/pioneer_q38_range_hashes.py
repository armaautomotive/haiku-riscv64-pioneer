#!/usr/bin/env python3
"""Read-only pinned model/transfer-range SHA manifest; never edits a model."""
import hashlib
import json
import os
import stat
import sys

TOTAL = 18_973_870_528
PUBLISHED = "c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
RANGES = [("prefix", 3_046_275_824)] + [
    (f"part{i}-startpos", 1_990_949_338) for i in range(7)
] + [("part7-startpos", 750_874_832), ("part7-buffered-suffix", 1_240_074_506)]


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage:pioneer_q38_range_hashes.py VERIFIED_MODEL")
    fd = os.open(sys.argv[1], os.O_RDONLY | os.O_NOFOLLOW)
    try:
        before = os.fstat(fd)
        if not stat.S_ISREG(before.st_mode) or before.st_size != TOTAL:
            raise SystemExit("STOP:exact regular model length required")
        whole = hashlib.sha256()
        offset = 0
        records = []
        for name, length in RANGES:
            digest = hashlib.sha256()
            remaining = length
            while remaining:
                data = os.read(fd, min(16 * 1024 * 1024, remaining))
                if not data:
                    raise SystemExit("STOP:short read")
                whole.update(data)
                digest.update(data)
                remaining -= len(data)
            records.append(dict(name=name, start=offset, length=length, sha256=digest.hexdigest()))
            offset += length
        after = os.fstat(fd)
        if (offset != TOTAL or whole.hexdigest() != PUBLISHED
                or (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns, before.st_ctime_ns)
                != (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns)):
            raise SystemExit("STOP:full SHA/source-stability failure;do not trust manifest")
        print(json.dumps(dict(full_sha256=PUBLISHED, bytes=TOTAL, ranges=records), indent=2))
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
