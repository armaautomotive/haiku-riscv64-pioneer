#!/usr/bin/env python3
"""Print apply_patch for exact frozen QPE1;validate BOTH files before output."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact QPE1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
if (len(code) != 3856 or hashlib.sha256(code).hexdigest() != 'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact frozen QPE1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_EMBEDDING_CODE_H', '#define PIONEER_Q38_EMBEDDING_CODE_H',
         '#include <stdint.h>', '// Frozen QPE1 DATA only,not execution/producer/source authority.',
         '// SHA256 c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1',
         'static const uint32_t PioneerQ38EmbeddingCode[]={']
words = struct.unpack('<964I', code)
for at in range(0, len(words), 8):
    lines.append(' ' + ','.join('0x%08xU' % word for word in words[at:at + 8]) + ',')
lines += ['};', 'static_assert(sizeof(PioneerQ38EmbeddingCode)==3856,"exact frozen QPE1");', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingCode.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
