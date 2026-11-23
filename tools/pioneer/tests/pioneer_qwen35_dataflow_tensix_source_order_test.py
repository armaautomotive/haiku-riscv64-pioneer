#!/usr/bin/env python3
"""Check the exported compute image orders each RISC-filled Tensix source."""

from pathlib import Path
import re
import sys


def check(disassembly):
    helper = re.search(
        r"<[^>]*DataflowL1StoresVisible[^>]*>:\n(.*?)(?=\n[0-9a-f]+ <|\Z)",
        disassembly,
        re.DOTALL,
    )
    if not helper:
        return False
    body = helper.group(1)
    if not re.search(r"fence\s+rw,rw.*?\blw\s+(\w+),[^\n]*\n[^\n]*\bmv\s+zero,\1", body, re.DOTALL):
        return False
    # These are the actual last readable words of the initialized source
    # tile, destination-init window, affine minima, and activation panel.
    for address in ("70a1c", "a03fc", "9127c", "7093c"):
        if not re.search(
            rf"# {address}\b[^\n]*\n[^\n]*\bauipc\b[^\n]*\n"
            r"[^\n]*\bjalr\b[^\n]*DataflowL1StoresVisible",
            disassembly,
        ):
            return False
    # The destination-init source is private to the projection. Its one
    # initialization and visibility fence must be in one-time Setup, not in
    # the output-group loop. Inspect the linked device image, not C++ text.
    setup = re.search(
        r"<[^>]*DataflowBlackholeTensixOps5SetupEv>:\n(.*?)(?=\n[0-9a-f]+ <|\Z)",
        disassembly,
        re.DOTALL,
    )
    if not setup or len(re.findall(r"# a03fc\b", disassembly)) != 1:
        return False
    if not re.search(r"# a03fc\b", setup.group(1)):
        return False
    return True


if len(sys.argv) != 2 or not check(Path(sys.argv[1]).read_text()):
    raise SystemExit("compute image lacks L1 source visibility or one-time destination initialization")
print("PASS compute image orders L1 sources and initializes the destination source once per projection; OFFLINE ONLY")
