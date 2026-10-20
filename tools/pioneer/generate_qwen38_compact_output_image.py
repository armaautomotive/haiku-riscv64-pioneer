#!/usr/bin/env python3
"""Emit an apply_patch addition for the pinned OFFLINE compact-output root image."""
import hashlib
import pathlib
import struct
import sys

assert len(sys.argv) == 2, 'Usage: generate_qwen38_compact_output_image.py IMAGE_DIRECTORY'
root = pathlib.Path(sys.argv[1])
pins = {
    'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
    'text': 'b0b67a77f5341604b0be5f759b4d9228aa934067a390e5805ab3b7c1de782194',
    'output': '1385ab712bf8756704a5c99abc344683b58b664a4bfac06443c2c83f7ae10427',
}
images = {}
for section, digest in pins.items():
    data = (root / (section + '.bin')).read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest, 'Wrong pinned ' + section
    assert data and not len(data) % 4 and len(data) <= 8192
    images[section] = data
assert struct.unpack('<I', images['boot'])[0] == 0x0006c06f

lines = [
    '/* SPDX-License-Identifier: MIT */',
    '#ifndef PIONEER_Q38_COMPACT_OUTPUT_ROOT_IMAGE_H',
    '#define PIONEER_Q38_COMPACT_OUTPUT_ROOT_IMAGE_H',
    '#include <stdint.h>',
    '// OFFLINE compact active-row exports;native qualification is separate.',
    '// Work keeps ALL current peer/source/counter/output/terminal/DMA proofs.',
    '// EVERY active result remains;unused per-worker padding is NOT transported.',
    '// Checked and STOP preserve their original full exports. Boot is unchanged.',
]
for section, function in [('text', 'Word'), ('output', 'PrefixWord')]:
    data = images[section]
    argument = 'word' if section == 'text' else 'address'
    lines += [
        '// ' + section + ' SHA256 ' + pins[section],
        'static inline uint32_t PioneerQ38CompactOutputRoot' + function + '(unsigned ' + argument + ')',
        '{',
        ' static const uint32_t code[]={',
    ]
    words = struct.unpack('<' + str(len(data) // 4) + 'I', data)
    for offset in range(0, len(words), 8):
        lines.append('  ' + ','.join('0x%08x' % word for word in words[offset:offset + 8]) + ',')
    lines += [
        ' };',
        ' static_assert(sizeof(code)==' + str(len(data)) + ' && sizeof(code)<=8192,"bounded immutable compact-output root");',
    ]
    if section == 'text':
        lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
    else:
        lines += [
            ' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
            ' return address-0x70000<sizeof(code)?code[(address-0x70000)/4]:0;',
        ]
    lines.append('}')
lines += ['#endif']
print('*** Begin Patch')
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38CompactOutputRootImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
