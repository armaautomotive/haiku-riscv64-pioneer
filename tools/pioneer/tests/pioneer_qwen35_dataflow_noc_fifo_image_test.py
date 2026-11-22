#!/usr/bin/env python3
"""Check exported RV32 role images actually sample FIFO availability/overflow."""
from pathlib import Path
import re
import sys


def check(path, noc, minimum):
    lines = Path(path).read_text().splitlines()
    found = 0
    for index, line in enumerate(lines):
        if f"ffb{noc}0064" not in line.lower() or "#" not in line:
            continue
        block = "\n".join(lines[index:index + 14]).lower()
        # Availability must be loaded and tested before the overflow read.
        overflow = block.find(f"ffb{noc}0068")
        if overflow < 0:
            continue
        prefix = block[:overflow]
        if re.search(r"\blw\s+", prefix) and re.search(r"\bbne\s+", prefix):
            found += 1
    if found < minimum:
        raise SystemExit(f"{path}: only {found} FIFO availability tests for NoC {noc}")


if len(sys.argv) != 4:
    raise SystemExit("usage: fifo-image-test reader writer graph")
check(sys.argv[1], 2, 2)  # quiet audit and transfer completion poll
check(sys.argv[2], 3, 2)
check(sys.argv[3], 2, 1)  # graph has separately callable checks
check(sys.argv[3], 3, 1)
print("PASS exported reader, writer and graph images test NoC FIFO availability; OFFLINE ONLY")
