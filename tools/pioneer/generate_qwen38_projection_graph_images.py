#!/usr/bin/env python3
"""Print one apply_patch addition for EXACT complete QPR1/QFP3 code DATA.

Validate BOTH boots and ALL six sections before printing ANY patch output.
Never write files,build,open SDKs,register a native role,map or launch.
"""
import hashlib
import pathlib
import struct
import sys

PINS = {
    'root-main': ('text', 7812, 'c623cad56856ac690a79e9b60566148afdba062646bafbb5bf32bcabaa7cb108', 'RootMain'),
    'root-prefix': ('prefix', 6508, 'cfb512c2b507d511b671bbb73101b724ec230f853a3486616336bacd66706357', 'RootPrefix'),
    'root-graph': ('graph', 16192, '13992bf95da5c695aab493db6d2760d74b139ed7384c04552df0751fa13ceaf9', 'RootGraph'),
    'peer-main': ('text', 8164, '783bd17e5f1114365bac3e7fa0589f205d70a225a2223a4de7089503ef90a615', 'PeerMain'),
    'peer-affine': ('affine', 5248, 'a55331eeab74b50a50012a62c0d476e9a247f653eefe2211a08d0a75b2da85ca', 'PeerAffine'),
    'peer-helper': ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9', 'PeerHelper'),
}


def main(args):
    if len(args) != 3 or args[2] not in PINS:
        raise ValueError('expected ROOT PEER and one exact complete QPR1/QFP3 section')
    images = {}
    for kind, path in [('root', pathlib.Path(args[0])), ('peer', pathlib.Path(args[1]))]:
        if (path / 'boot.bin').read_bytes() != struct.pack('<I', 0x0006c06f):
            raise ValueError('exact shared boot mismatch')
        for name, (section, size, pin, _) in PINS.items():
            if not name.startswith(kind + '-'):
                continue
            data = (path / (section + '.bin')).read_bytes()
            if len(data) != size or len(data) % 4 or hashlib.sha256(data).hexdigest() != pin:
                raise ValueError('complete pinned QPR1/QFP3 section mismatch: ' + name)
            images[name] = data
    name = args[2]
    _, size, pin, suffix = PINS[name]
    guard = 'PIONEER_Q38_PROJECTION_GRAPH_' + suffix.upper() + '_IMAGE_H'
    symbol = 'PioneerQ38ProjectionGraph' + suffix
    lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef ' + guard, '#define ' + guard,
             '#include <stdint.h>', '// Frozen complete QPR1/QFP3 DATA,NOT native/source/producer/owner authority.',
             '// SHA256 ' + pin, 'static const uint32_t ' + symbol + '[]={']
    words = struct.unpack('<' + str(size // 4) + 'I', images[name])
    for at in range(0, len(words), 8):
        lines.append(' ' + ','.join('0x%08xU' % value for value in words[at:at+8]) + ',')
    lines += ['};', 'static_assert(sizeof(' + symbol + ')==' + str(size) + ',"exact complete QPR1/QFP3 section");', '#endif']
    print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/' + symbol + '.h')
    for line in lines:
        print('+' + line)
    print('*** End Patch')


if __name__ == '__main__':
    main(sys.argv[1:])
