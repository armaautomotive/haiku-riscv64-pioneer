#!/usr/bin/env python3
"""Print apply_patch ONLY for the exact qualified QGI1 ROM DATA."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact qualified QGI1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
if (len(code) != 8404 or hashlib.sha256(code).hexdigest() != 'bcc15ddc0e76c0a432a63502201f9c2451366b02615e3cddc9cf64bb147757c1'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact qualified QGI1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_GDN_INPUT_IMAGE_H', '#define PIONEER_Q38_GDN_INPUT_IMAGE_H',
         '#include <stdint.h>', '// Frozen QGI1 ROM DATA,NOT source/producer/initialized-state/native authority.',
         '// Code8404B SHA256 bcc15ddc0e76c0a432a63502201f9c2451366b02615e3cddc9cf64bb147757c1',
         'enum {TT_Q38_GDN_INPUT_TEXT_BYTES=8404,TT_Q38_GDN_INPUT_IMAGE_WORDS=2102};',
         'static inline uint32_t PioneerQ38GdnInputImageWord(unsigned i){', ' static const uint32_t code[2101]={']
words = struct.unpack('<2101I', code)
for at in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08xU' % word for word in words[at:at + 8]) + ',')
lines += [' };', ' return i>=TT_Q38_GDN_INPUT_IMAGE_WORDS?UINT32_MAX:!i?0x0005e06f:code[i-1];', '}',
          'static inline uint32_t PioneerQ38GdnInputImageAddress(unsigned i){return !i?0x6000:0x64000+(i-1)*4;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38GdnInputImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
