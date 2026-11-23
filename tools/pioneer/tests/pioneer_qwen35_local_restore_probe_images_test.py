#!/usr/bin/env python3
"""Verify the probe header embeds exactly the linked, relocated images."""
import re
import struct
import sys
from pathlib import Path

if len(sys.argv) != 2:
    raise SystemExit('usage: test.py IMAGE_DIRECTORY')
folder = Path(sys.argv[1])
header = (folder / 'PioneerQwen35LocalRestoreProbeImages.h').read_text(encoding='ascii')
for role, code_address in (('reader', 0xB0000), ('writer', 0xB5000)):
    cap = role.capitalize()
    for kind, binary in (('Shim', folder / f'{role}-shim.bin'),
                         ('Code', folder / 'local-restore' / f'{role}.bin')):
        match = re.search(rf'DataflowLocalProbe{cap}{kind}Words\[\]=\{{(.*?)\}};', header, re.DOTALL)
        assert match
        words = [int(x, 16) for x in re.findall(r'0x([0-9a-f]{8})U', match.group(1))]
        assert b''.join(struct.pack('<I', word) for word in words) == binary.read_bytes()
    park = re.search(rf'DataflowLocalProbe{cap}ParkAddress=0x([0-9a-f]{{8}})U', header)
    assert park
    restore = (folder / 'local-restore' / f'{role}.bin').read_bytes()
    offset = int(park.group(1), 16) - code_address
    assert 0 <= offset <= len(restore)-4 and not offset % 4
    assert struct.unpack_from('<I', restore, offset)[0] == 0x6f
assert len(re.findall(r'(?:Shim|Code)Words\[\]', header)) == 4
print('PASS restricted probe header contains the exact linked shim and restore bytes; OFFLINE ONLY')
