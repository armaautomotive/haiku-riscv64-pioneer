#!/usr/bin/env python3
"""Emit an apply_patch addition for the exact offline-tested local GDN image."""
import hashlib
import pathlib
import struct
import sys

assert len(sys.argv) == 2
root = pathlib.Path(sys.argv[1])
code = (root / 'code.bin').read_bytes()
boot = (root / 'boot.bin').read_bytes()
assert hashlib.sha256(code).hexdigest() == '0c61dc435a3c2e00c8ae8a11319a307c86707b410100f7fbc85d15f8587bfcbe'
assert hashlib.sha256(boot).hexdigest() == 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
assert struct.unpack('<I', boot)[0] == 0x0006c06f
assert 0 < len(code) <= 8192 and not len(code) % 4
words = struct.unpack('<' + str(len(code) // 4) + 'I', code)
lines = [
    '/* SPDX-License-Identifier: MIT */',
    '#ifndef PIONEER_Q38_POSTSCALE_DELTA_IMAGE_H',
    '#define PIONEER_Q38_POSTSCALE_DELTA_IMAGE_H',
    '// Separate ABI3 LOCAL qualification candidate:raw Q,post-dot scale.',
    '// text SHA256 0c61dc435a3c2e00c8ae8a11319a307c86707b410100f7fbc85d15f8587bfcbe',
    '// boot SHA256 e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
    'enum { TT_Q38_DELTA_BOOT=0x0006c06f,TT_Q38_DELTA_CODE=0x72000,',
    ' TT_Q38_DELTA_CODE_BYTES=' + str(len(code)) + ',TT_Q38_DELTA_DONE=0x7200c,TT_Q38_DELTA_PARK=0x72038 };',
    'static inline uint32_t PioneerQ38DeltaProbeWord(unsigned word)',
    '{',
    ' static const uint32_t words[]={',
]
for first in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08xU' % w for w in words[first:first + 8]) + ',')
lines += [' };', ' static_assert(sizeof(words)==TT_Q38_DELTA_CODE_BYTES,"exact pinned postscale image");',
          ' return word<sizeof(words)/4?words[word]:0;', '}', '#endif']
print('*** Begin Patch')
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38PostscaleDeltaImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
