#!/usr/bin/env python3
"""Print apply_patch ONLY for exact qualified QGH1 ROM DATA,no device grant."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact qualified QGH1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
if (len(code) != 23020 or hashlib.sha256(code).hexdigest() != '22e3001215d67c9b9f8861098db5e5fda76fb241076c0ece8adb3ba224dfe297'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact qualified QGH1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_GDN_GATED_IMAGE_H', '#define PIONEER_Q38_GDN_GATED_IMAGE_H',
         '#include <stdint.h>', '// Frozen QGH1 ROM DATA,NOT source/initialized-state/native authority.',
         '// Code23020B SHA256 22e3001215d67c9b9f8861098db5e5fda76fb241076c0ece8adb3ba224dfe297',
         'enum {TT_Q38_GDN_GATED_TEXT_BYTES=23020,TT_Q38_GDN_GATED_IMAGE_WORDS=5756};',
         'static inline uint32_t PioneerQ38GdnGatedImageWord(unsigned i){', ' static const uint32_t code[5755]={']
words = struct.unpack('<5755I', code)
for at in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08xU' % word for word in words[at:at + 8]) + ',')
lines += [' };', ' return i>=TT_Q38_GDN_GATED_IMAGE_WORDS?UINT32_MAX:!i?0x0005e06f:code[i-1];', '}',
          'static inline uint32_t PioneerQ38GdnGatedImageAddress(unsigned i){return !i?0x6000:0x64000+(i-1)*4;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38GdnGatedImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
