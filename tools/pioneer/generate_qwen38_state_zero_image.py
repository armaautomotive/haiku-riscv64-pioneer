#!/usr/bin/env python3
"""Print an apply_patch for the pinned OFFLINE initializer;no file writes."""
import hashlib
import pathlib
import struct
import sys

assert len(sys.argv) == 2
root = pathlib.Path(sys.argv[1])
boot = (root / 'boot.bin').read_bytes()
code = (root / 'text.bin').read_bytes()
assert hashlib.sha256(boot).hexdigest() == 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
assert hashlib.sha256(code).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
assert len(boot) == 4 and struct.unpack('<I', boot)[0] == 0x0006c06f
assert code and len(code) <= 8192 and not len(code) % 4
lines = [
    '/* SPDX-License-Identifier: MIT */',
    '#ifndef PIONEER_Q38_STATE_ZERO_IMAGE_H',
    '#define PIONEER_Q38_STATE_ZERO_IMAGE_H',
    '#include <stdint.h>',
    '// OFFLINE prerequisite ONLY;NO native driver operation selects this image.',
    '// Device-only reserved GDDR zero+poisoned readback;NO host address/BME.',
    '// Code SHA25639817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b.',
    'enum { TT_Q38_STATE_ZERO_BOOT=0x0006c06f };',
    'static inline uint32_t PioneerQ38StateZeroCodeWord(unsigned word)',
    '{',
    ' static const uint32_t code[]={',
]
words = struct.unpack('<' + str(len(code) // 4) + 'I', code)
for at in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08x' % word for word in words[at:at + 8]) + ',')
lines += [
    ' };',
    ' static_assert(sizeof(code)==' + str(len(code)) + ' && sizeof(code)<=8192,"bounded immutable state-zero image");',
    ' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;',
    '}',
    '#endif',
]
print('*** Begin Patch')
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38StateZeroImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
