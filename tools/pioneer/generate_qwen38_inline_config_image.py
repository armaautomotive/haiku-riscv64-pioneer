#!/usr/bin/env python3
"""Emit only an apply_patch addition for the exact tested peer; no writes."""
import hashlib
import pathlib
import struct
import sys

assert len(sys.argv) == 2, 'Usage: generate_qwen38_inline_config_image.py IMAGE_DIRECTORY'
root = pathlib.Path(sys.argv[1])
pins = {
    'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
    'text': 'b86c3bc1cdf645f60b22e233d7b7f23033791645b4ae5a2edc0a9b9c92653cc3',
    'affine': 'fcfb6af634be07c7abbd33ef45d328361d5c4769f3e73bb842bc9f68bcfda38d',
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
    '#ifndef PIONEER_Q38_INLINE_CONFIG_PEER_IMAGE_H',
    '#define PIONEER_Q38_INLINE_CONFIG_PEER_IMAGE_H',
    '#include <stdint.h>',
    '// Separate candidate;native qualification is NOT an offline step count.',
    '// SAME ordered volatile CSR write/readback,emitted in the hot caller.',
    '// No config/value/drain/math/NoC/check/publication change from deferred.',
]
for section, function in [('text', 'Word'), ('affine', 'PrefixWord')]:
    data = images[section]
    argument = 'word' if section == 'text' else 'address'
    lines += [
        '// ' + section + ' SHA256 ' + pins[section],
        'static inline uint32_t PioneerQ38InlineConfigPeer' + function + '(unsigned ' + argument + ')',
        '{',
        ' static const uint32_t code[]={',
    ]
    words = struct.unpack('<' + str(len(data) // 4) + 'I', data)
    for offset in range(0, len(words), 8):
        lines.append('  ' + ','.join('0x%08x' % word for word in words[offset:offset + 8]) + ',')
    lines += [
        ' };',
        ' static_assert(sizeof(code)==' + str(len(data)) + ' && sizeof(code)<=' +
        str(8192 if section == 'text' else 0x1500) + ',"bounded immutable inline-config peer");',
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
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38InlineConfigPeerImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
