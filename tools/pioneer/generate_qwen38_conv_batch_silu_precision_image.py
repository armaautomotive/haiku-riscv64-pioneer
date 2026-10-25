#!/usr/bin/env python3
"""Print apply_patch only for the exact OFFLINE-qualified precision image."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen precise QCB1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root/'code.bin').read_bytes(), (root/'boot.bin').read_bytes()
if (len(code) != 7560 or hashlib.sha256(code).hexdigest() != '0a009317f69ecba0d6f56575508d4d4e6bdb3c30ece3e1f78a85cd66e2ab644c'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact frozen precise QCB1 code/boot mismatch')
name = 'PioneerQ38ConvBatchSiluPrecision'
guard = 'PIONEER_Q38_CONV_BATCH_SILU_PRECISION_IMAGE_H'
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef '+guard, '#define '+guard, '#include <stdint.h>',
         '// Frozen OFFLINE precise QCB1 DATA only;NOT a native selector/source/launch capability.',
         '// SHA256 0a009317f69ecba0d6f56575508d4d4e6bdb3c30ece3e1f78a85cd66e2ab644c',
         'static inline uint32_t '+name+'CodeWord(unsigned i){', ' static const uint32_t code[1890]={']
words = struct.unpack('<1890I', code)
for at in range(0, len(words), 8):
    lines.append('  '+','.join('0x%08xU' % word for word in words[at:at+8])+',')
lines += [' };', ' return i<1890?code[i]:UINT32_MAX;', '}',
          'enum {TT_Q38_CONV_BATCH_SILU_PRECISION_TEXT_BYTES=7560,TT_Q38_CONV_BATCH_SILU_PRECISION_IMAGE_WORDS=1891};',
          'static inline uint32_t '+name+'ImageAddress(unsigned i){return !i?0x6000:i<1891?0x64000+(i-1)*4:UINT32_MAX;}',
          'static inline uint32_t '+name+'ImageWord(unsigned i){return !i?0x0005e06fU:'+name+'CodeWord(i-1);}',
          'static inline bool '+name+'TerminalPc(uint32_t pc){return pc==0x64038;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+name+'Image.h')
for line in lines:
    print('+'+line)
print('*** End Patch')
