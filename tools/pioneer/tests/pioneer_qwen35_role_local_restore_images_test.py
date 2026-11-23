#!/usr/bin/env python3
"""Verify the exported host header matches the linked restore shim bytes."""
import re
import struct
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit('usage: test.py IMAGE_DIRECTORY')
folder = Path(sys.argv[1])
header = (folder / 'PioneerQwen35LocalRestoreImages.h').read_text(encoding='ascii')
for role, address in (('reader', 0x70000), ('writer', 0x70100)):
    cap = role.capitalize()
    pattern = rf'DataflowLocalRestore{cap}ShimWords\[\]=\{{(.*?)\}};'
    match = re.search(pattern, header, re.DOTALL)
    assert match
    words = [int(value, 16) for value in re.findall(r'0x([0-9a-f]{8})U', match.group(1))]
    binary = (folder / f'{role}.bin').read_bytes()
    assert b''.join(struct.pack('<I', word) for word in words) == binary
    park_match = re.search(rf'DataflowLocalRestore{cap}ParkAddress=0x([0-9a-f]{{8}})U', header)
    assert park_match
    park = int(park_match.group(1), 16)
    offset = park - address
    assert offset >= 0 and offset % 4 == 0 and offset + 4 <= len(binary)
    assert struct.unpack_from('<I', binary, offset)[0] == 0x6f
assert len(re.findall(r'ShimWords\[\]', header)) == 2
print('PASS local restore header matches linked images and park PCs; OFFLINE ONLY')
