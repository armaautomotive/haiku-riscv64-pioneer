#!/usr/bin/env python3
"""Print apply_patch ONLY for qualified timed QGT1 ROM DATA,no device grant."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact qualified QGT1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
if (len(code) != 23792 or hashlib.sha256(code).hexdigest() != '3a99ac89d98236b7e3dac5e1cf433223961fdb1b7f0f6f80dd81d541a47b0284'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact qualified QGT1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_GDN_RENDEZVOUS_IMAGE_H', '#define PIONEER_Q38_GDN_RENDEZVOUS_IMAGE_H',
         '#include <stdint.h>', '// Frozen QGT1 ROM DATA,NOT source/initialized-state/native authority.',
         '// Code23792B SHA256 3a99ac89d98236b7e3dac5e1cf433223961fdb1b7f0f6f80dd81d541a47b0284',
         'enum {TT_Q38_GDN_RENDEZVOUS_TEXT_BYTES=23792,TT_Q38_GDN_RENDEZVOUS_IMAGE_WORDS=5949};',
         'static inline uint32_t PioneerQ38GdnRendezvousImageWord(unsigned i){', ' static const uint32_t code[5948]={']
words = struct.unpack('<5948I', code)
for at in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08xU' % word for word in words[at:at + 8]) + ',')
lines += [' };', ' return i>=TT_Q38_GDN_RENDEZVOUS_IMAGE_WORDS?UINT32_MAX:!i?0x0005e06f:code[i-1];', '}',
          'static inline uint32_t PioneerQ38GdnRendezvousImageAddress(unsigned i){return !i?0x6000:0x64000+(i-1)*4;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38GdnRendezvousImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
