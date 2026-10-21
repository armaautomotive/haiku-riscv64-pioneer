#!/usr/bin/env python3
"""Emit an apply_patch addition for the exact offline-tested ROOT0 image."""
import hashlib
import pathlib
import struct
import sys
assert len(sys.argv) == 2
root = pathlib.Path(sys.argv[1])
code, boot = (root / 'code.bin').read_bytes(), (root / 'boot.bin').read_bytes()
assert hashlib.sha256(code).hexdigest() == '518fbdefed49df2afd89837e6fd2edb08d0bf0c33df5a55e47ccb3ccbc21951b'
assert hashlib.sha256(boot).hexdigest() == 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
assert struct.unpack('<I', boot)[0] == 0x0006c06f and len(code) == 5928
words = struct.unpack('<' + str(len(code) // 4) + 'I', code)
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_RESIDENT_GDN_ROOT_IMAGE_H',
         '#define PIONEER_Q38_RESIDENT_GDN_ROOT_IMAGE_H',
         '// Separate ROOT0/GDDR-readback DIAGNOSTIC,NEVER Work selection.',
         '// text SHA256 518fbdefed49df2afd89837e6fd2edb08d0bf0c33df5a55e47ccb3ccbc21951b',
         '// boot SHA256 e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
         'enum { TT_Q38_RGDN_ROOT_BOOT=0x0006c06f,TT_Q38_RGDN_ROOT_CODE_BYTES=5928 };',
         'static inline uint32_t PioneerQ38ResidentGdnRootWord(unsigned word)', '{', ' static const uint32_t code[]={']
for first in range(0, len(words), 8):
    lines.append('  ' + ','.join('0x%08xU' % value for value in words[first:first + 8]) + ',')
lines += [' };', ' static_assert(sizeof(code)==TT_Q38_RGDN_ROOT_CODE_BYTES,"exact bounded ROOT0 image");',
          ' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;', '}', '#endif']
print('*** Begin Patch')
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRootImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
