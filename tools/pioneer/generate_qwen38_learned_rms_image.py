#!/usr/bin/env python3
"""Print apply_patch ONLY after exact immutable QRN1 code+boot verification."""
import hashlib
import pathlib
import struct
import sys
if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen QRN1 image directory')
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
if (len(code) != 5052 or hashlib.sha256(code).hexdigest() != 'f13d8676f97920c182fa47d9f555453d7855dd5ac394919a577d58c83abe523a'
        or boot != struct.pack('<I', 0x0005e06f)):
    raise SystemExit('exact frozen QRN1 code/boot mismatch')
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_LEARNED_RMS_CODE_H', '#define PIONEER_Q38_LEARNED_RMS_CODE_H',
         '#include <stdint.h>', '// Frozen QRN1 DATA,not source/input/producer/native execution authority.',
         '// SHA256 f13d8676f97920c182fa47d9f555453d7855dd5ac394919a577d58c83abe523a',
         'static const uint32_t PioneerQ38LearnedRmsCode[]={']
words = struct.unpack('<1263I', code)
for at in range(0, len(words), 8):
    lines.append(' ' + ','.join('0x%08xU' % word for word in words[at:at + 8]) + ',')
lines += ['};', 'static_assert(sizeof(PioneerQ38LearnedRmsCode)==5052,"exact frozen QRN1");',
          'static inline uint32_t PioneerQ38LearnedRmsProgramWord(unsigned word){return word>10240?UINT32_MAX:!word?0x0005e06f:word<=1263?PioneerQ38LearnedRmsCode[word-1]:0;}', '#endif']
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38LearnedRmsCode.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
