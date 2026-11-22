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
    return True


if len(sys.argv) != 2 or not check(Path(sys.argv[1]).read_text()):
    raise SystemExit("compute image lacks L1 source visibility before Tensix access")
print("PASS compute image orders four L1 source regions before Tensix use; OFFLINE ONLY")
