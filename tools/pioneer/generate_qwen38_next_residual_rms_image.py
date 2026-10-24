#!/usr/bin/env python3
"""Print apply_patch ONLY after exact immutable QRR2 image/boot checks."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen QRR2 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root/'code.bin').read_bytes(), (root/'boot.bin').read_bytes()
if (len(code) != 5512 or hashlib.sha256(code).hexdigest() != 'f2fdcb0becf99281ba6910dbb64551d32584c0cc054bc452f030217ea5e81636'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact frozen QRR2 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_NEXT_RESIDUAL_RMS_CODE_H', '#define PIONEER_Q38_NEXT_RESIDUAL_RMS_CODE_H',
         '#include <stdint.h>', '// Frozen QRR2 DATA only;NOT a source/producer/native launch capability.',
         '// SHA256 f2fdcb0becf99281ba6910dbb64551d32584c0cc054bc452f030217ea5e81636',
         'static const uint32_t PioneerQ38NextResidualRmsCode[]={']
words = struct.unpack('<1378I', code)
for at in range(0, len(words), 8):
    lines.append(' '+','.join('0x%08xU' % word for word in words[at:at+8])+',')
lines += ['};', 'static_assert(sizeof(PioneerQ38NextResidualRmsCode)==5512,"exact frozen QRR2");',
          'static inline uint32_t PioneerQ38NextResidualRmsProgramWord(unsigned word){return word>10240?UINT32_MAX:!word?0x0005e06f:word<=1378?PioneerQ38NextResidualRmsCode[word-1]:0;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38NextResidualRmsCode.h')
for line in lines:
    print('+'+line)
print('*** End Patch')
