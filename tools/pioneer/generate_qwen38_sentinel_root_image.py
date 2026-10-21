#!/usr/bin/env python3
"""Print an apply_patch addition for the independently tested offline sentinel-poll root."""
import hashlib
import pathlib
import struct
import sys
assert len(sys.argv) == 2
root = pathlib.Path(sys.argv[1])
pins = {'text': '65c790cf68dc705a3af5458e2b89aafa28a5b316c0a7b9fad1c8bd51fd36ecd8',
        'output': 'd358b793b2c83def15e58fa52b62197e2f2241dd85e9bc1a5fc9a1733cf46308',
        'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images = {}
for section, pin in pins.items():
    data = (root / (section + '.bin')).read_bytes()
    assert data and len(data) % 4 == 0 and len(data) <= 8192
    assert hashlib.sha256(data).hexdigest() == pin
    images[section] = data
assert struct.unpack('<I', images['boot'])[0] == 0x0006c06f
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_SENTINEL_ROOT_IMAGE_H',
         '#define PIONEER_Q38_SENTINEL_ROOT_IMAGE_H', '#include <stdint.h>',
         '// Separate OFFLINE sentinel/backoff root;ALL original per-worker terminal audits retained.']
for section, suffix in [('text', 'Word'), ('output', 'PrefixWord')]:
    data = images[section]
    argument = 'word' if section == 'text' else 'address'
    lines += ['// ' + section + ' SHA256 ' + pins[section],
              'static inline uint32_t PioneerQ38SentinelRoot' + suffix + '(unsigned ' + argument + ')',
              '{', ' static const uint32_t code[]={']
    words = struct.unpack('<' + str(len(data) // 4) + 'I', data)
    for offset in range(0, len(words), 8):
        lines.append('  ' + ','.join('0x%08xU' % w for w in words[offset:offset+8]) + ',')
    lines += [' };', ' static_assert(sizeof(code)==' + str(len(data)) + ' && sizeof(code)<=8192,"bounded exact sentinel root");']
    if section == 'text':
        lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
    else:
        lines += [' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
                  ' return address-0x70000<sizeof(code)?code[(address-0x70000)/4]:0;']
    lines.append('}')
lines.append('#endif')
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38SentinelRootImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
