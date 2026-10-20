#!/usr/bin/env python3
"""Emit an apply_patch addition for an exact OFFLINE peer; no file writes."""
import hashlib
import pathlib
import struct
import sys

assert len(sys.argv) == 2, 'Usage: generate_qwen38_input_sum_cache_image.py IMAGE_DIRECTORY'
root = pathlib.Path(sys.argv[1])
pins = {
    'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
    'text': 'c2f5dbf162261221c0cdfafe45e24f778aaae32b180dbaa01453f6c4881f87d0',
    'affine': 'f5b36fa25338b9e63f8a10f0cf6bb60cc397e91261bfececcec603eab1b94250',
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
    '#ifndef PIONEER_Q38_INPUT_SUM_CACHE_PEER_IMAGE_H',
    '#define PIONEER_Q38_INPUT_SUM_CACHE_PEER_IMAGE_H',
    '#include <stdint.h>',
    '// OFFLINE candidate; native qualification is separate from image tests.',
    '// Work group0 defines ALL exact integer sums anew on EVERY job.',
    '// Later groups reuse private scratch95000..96100,never cross-job hits.',
    '// Checked/single-group retain original arithmetic. No NoC/proof changes.',
]
for section, function in [('text', 'Word'), ('affine', 'PrefixWord')]:
    data = images[section]
    argument = 'word' if section == 'text' else 'address'
    lines += [
        '// ' + section + ' SHA256 ' + pins[section],
        'static inline uint32_t PioneerQ38InputSumCachePeer' + function + '(unsigned ' + argument + ')',
        '{',
        ' static const uint32_t code[]={',
    ]
    words = struct.unpack('<' + str(len(data) // 4) + 'I', data)
    for offset in range(0, len(words), 8):
        lines.append('  ' + ','.join('0x%08x' % word for word in words[offset:offset + 8]) + ',')
    lines += [
        ' };',
        ' static_assert(sizeof(code)==' + str(len(data)) + ' && sizeof(code)<=' +
        str(8192 if section == 'text' else 0x1500) + ',"bounded immutable sum-cache peer");',
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
print('*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38InputSumCachePeerImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
