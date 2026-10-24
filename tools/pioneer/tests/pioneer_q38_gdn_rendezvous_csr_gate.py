#!/usr/bin/env python3
"""Audit actual opcode bits,not mnemonic spelling. Read mcycle/h ONLY."""
import pathlib
import re
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected actual disassembly')
seen = 0
for line in pathlib.Path(sys.argv[1]).read_text().splitlines():
    match = re.match(r'\s*[0-9a-f]+:\s+([0-9a-f]{8})\s', line)
    if not match:
        continue
    op = int(match[1], 16)
    if op & 127 != 0x73:
        continue
    assert (op >> 12) & 7 == 2 and (op >> 15) & 31 == 0 and op >> 20 in (0xb00, 0xb80) and (op >> 7) & 31, hex(op)
    seen += 1
assert seen == 3, seen
print('PASS actual QGT1 opcode audit:3 READ-only CSRRS mcycleh/mcycle/mcycleh,x0;NO CSR/control/counter writes')
