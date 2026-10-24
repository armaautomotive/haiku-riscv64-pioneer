#!/usr/bin/env python3
"""Print apply_patch ONLY after exact immutable QRR1 image/boot checks."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen QRR1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root/'code.bin').read_bytes(), (root/'boot.bin').read_bytes()
if (len(code) != 5468 or hashlib.sha256(code).hexdigest() != '93976e38dfc9f310a7415ce83bc6cabc2ebf5c15ac10c1745fee79d1f2c61989'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact frozen QRR1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_RESIDUAL_RMS_CODE_H', '#define PIONEER_Q38_RESIDUAL_RMS_CODE_H',
         '#include <stdint.h>', '// Frozen QRR1 DATA only;NOT a source/producer/native launch capability.',
         '// SHA256 93976e38dfc9f310a7415ce83bc6cabc2ebf5c15ac10c1745fee79d1f2c61989',
         'static const uint32_t PioneerQ38ResidualRmsCode[]={']
words = struct.unpack('<1367I', code)
for at in range(0, len(words), 8):
    lines.append(' '+','.join('0x%08xU' % word for word in words[at:at+8])+',')
lines += ['};', 'static_assert(sizeof(PioneerQ38ResidualRmsCode)==5468,"exact frozen QRR1");',
          'static inline uint32_t PioneerQ38ResidualRmsProgramWord(unsigned word){return word>10240?UINT32_MAX:!word?0x0005e06f:word<=1367?PioneerQ38ResidualRmsCode[word-1]:0;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38ResidualRmsCode.h')
for line in lines:
    print('+'+line)
print('*** End Patch')
