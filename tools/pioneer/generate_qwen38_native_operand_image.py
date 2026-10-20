#!/usr/bin/env python3
"""Emit an apply_patch addition for an exact OFFLINE peer; no file writes."""
import hashlib
import pathlib
import struct
import sys

assert len(sys.argv) == 2, 'Usage: generate_qwen38_native_operand_image.py IMAGE_DIRECTORY'
root = pathlib.Path(sys.argv[1])
pins = {
    'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
    'text': '8fd633343fc76240cb36fe12a1c58080875550cf1719139f07df55f9915f8c8b',
    'affine': 'deb5d7c79bb9bdeb6a17284e4100635545471d484e8e509e559b97dde0bc1983',
}
images = {}
for section, digest in pins.items():
    data = (root / (section + '.bin')).read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest, 'Wrong pinned ' + section
    assert data and not len(data) % 4 and len(data) <= 8192
    images[section] = data
assert struct.unpack('<I', images['boot'])[0] == 0x0006c06f
assert len(images['affine']) <= 0x1500

lines = [
    '/* SPDX-License-Identifier: MIT */',
    '#ifndef PIONEER_Q38_NATIVE_OPERAND_PEER_IMAGE_H',
    '#define PIONEER_Q38_NATIVE_OPERAND_PEER_IMAGE_H',
    '#include <stdint.h>',
    '// OFFLINE candidate; native qualification is separate from image tests.',
    '// Native mode5 writes ONLY existing decoded HIGH physical halves.',
    '// Initialized LOW halves remain ZERO;MOV/CSR/format are unchanged.',
    '// Same exact decoded integers/all math;no NoC/proof changes.',
]
for section, function in [('text', 'Word'), ('affine', 'PrefixWord')]:
    data = images[section]
    argument = 'word' if section == 'text' else 'address'
    lines += [
        '// ' + section + ' SHA256 ' + pins[section],
        'static inline uint32_t PioneerQ38NativeOperandPeer' + function + '(unsigned ' + argument + ')',
        '{',
        ' static const uint32_t code[]={',
    ]
    words = struct.unpack('<' + str(len(data) // 4) + 'I', data)
    for offset in range(0, len(words), 8):
        lines.append('  ' + ','.join('0x%08x' % word for word in words[offset:offset + 8]) + ',')
    lines += [
        ' };',
        ' static_assert(sizeof(code)==' + str(len(data)) + ' && sizeof(code)<=' +
        str(8192 if section == 'text' else 0x1500) + ',"bounded immutable native-operand peer");',
    ]
    if section == 'text':
        lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
    else:
        lines += [
            ' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
            ' return address>=0x70b00 && address-0x70b00<sizeof(code)?code[(address-0x70b00)/4]:0;',
        ]
    lines.append('}')
lines += ['#endif']
print('*** Begin Patch')
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38NativeOperandPeerImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
