#!/usr/bin/env python3
"""Print an apply_patch addition for the independently tested timing root."""
import hashlib
import pathlib
import struct
import sys
assert len(sys.argv) == 2
root = pathlib.Path(sys.argv[1])
pins = {'text': 'aa27f4356c7cc6bfb14dabbf79473a6b8e519ff0386ca343b4e7eabd500de5f7',
        'output': '15643b20c5aafa9090c00e6be0dd4985653cae7d621c707c3f69e27ddde9d03d',
        'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'}
images = {}
for section, pin in pins.items():
    data = (root / (section + '.bin')).read_bytes()
    assert data and len(data) % 4 == 0 and len(data) <= 8192
    assert hashlib.sha256(data).hexdigest() == pin
    images[section] = data
assert struct.unpack('<I', images['boot'])[0] == 0x0006c06f
lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef PIONEER_Q38_ROOT_CYCLE_IMAGE_H',
         '#define PIONEER_Q38_ROOT_CYCLE_IMAGE_H', '#include <stdint.h>',
         '// Opt-in DIAGNOSTIC root;read-only mcycle/h,NO extra NoC/DMA/authority.']
for section, suffix in [('text', 'Word'), ('output', 'PrefixWord')]:
    data = images[section]
    argument = 'word' if section == 'text' else 'address'
    lines += ['// ' + section + ' SHA256 ' + pins[section],
              'static inline uint32_t PioneerQ38RootCycle' + suffix + '(unsigned ' + argument + ')',
              '{', ' static const uint32_t code[]={']
    words = struct.unpack('<' + str(len(data) // 4) + 'I', data)
    for offset in range(0, len(words), 8):
        lines.append('  ' + ','.join('0x%08xU' % w for w in words[offset:offset+8]) + ',')
    lines += [' };', ' static_assert(sizeof(code)==' + str(len(data)) + ' && sizeof(code)<=8192,"bounded exact cycle root");']
    if section == 'text':
        lines.append(' return word<sizeof(code)/4?code[word]:word<2048?0:UINT32_MAX;')
    else:
        lines += [' if (address<0x70000 || address>=0x72000 || (address&3)) return UINT32_MAX;',
                  ' return address-0x70000<sizeof(code)?code[(address-0x70000)/4]:0;']
    lines.append('}')
lines.append('#endif')
print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleImage.h')
for line in lines:
    print('+' + line)
print('*** End Patch')
