#!/usr/bin/env python3
"""Print apply_patch ONLY for exact qualified QRJ1 ROM DATA,no device grant."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact qualified QRJ1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
if (len(code) != 14836 or hashlib.sha256(code).hexdigest() != '96881b6501e4febea5ed869c1e30e26a8bb432ce45001b7cd1fe93234e50a410'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact qualified QRJ1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_GDN_RECURRENT_IMAGE_H', '#define PIONEER_Q38_GDN_RECURRENT_IMAGE_H',
         '#include <stdint.h>', '// Frozen QRJ1 ROM DATA,NOT source/initialized-state/native authority.',
         '// Code14836B SHA256 96881b6501e4febea5ed869c1e30e26a8bb432ce45001b7cd1fe93234e50a410',
         'enum {TT_Q38_GDN_RECURRENT_TEXT_BYTES=14836,TT_Q38_GDN_RECURRENT_IMAGE_WORDS=3710};',
         'static inline uint32_t PioneerQ38GdnRecurrentImageWord(unsigned i){', ' static const uint32_t code[3709]={']
words = struct.unpack('<3709I', code)
for at in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08xU' % word for word in words[at:at + 8]) + ',')
lines += [' };', ' return i>=TT_Q38_GDN_RECURRENT_IMAGE_WORDS?UINT32_MAX:!i?0x0005e06f:code[i-1];', '}',
          'static inline uint32_t PioneerQ38GdnRecurrentImageAddress(unsigned i){return !i?0x6000:0x64000+(i-1)*4;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38GdnRecurrentImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
