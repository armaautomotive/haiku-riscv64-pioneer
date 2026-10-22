#!/usr/bin/env python3
"""Freeze pinned QFG2 ROOT data ONLY,never replace the QFG1 catalog.

ALL root/peer sections and both boots verify before ANY patch output.
No file writes,SDK/build/native selection or launch authority.
"""
import hashlib
import pathlib
import struct
import sys

PINS = {
    'root-main': ('text', 6884, 'a363fb40ba949badda8ba8c863385484bee9cd7c52e1cf0a7ef587b029abcba9', 'RootMain'),
    'root-prefix': ('prefix', 6172, '255ff2bb850e0e30a4a0e08dd501a245eb0d021d70302e58c0b5ae014dde8740', 'RootPrefix'),
    'root-graph': ('graph', 17688, 'a0b2fdf23c3e5eead311e79a80989ea113dbe9fcad7c0033a281381d23bdd998', 'RootGraph'),
    'peer-main': ('text', 8180, 'd2052fbbf43dcc022747ea2b228e7b1bc0f829f4984821ab4dfca20473e3a8ef', 'PeerMain'),
    'peer-affine': ('affine', 5152, '96e92065722a477b08ba41abc2f623064d44970c5807b37c6d739e8dc3697570', 'PeerAffine'),
    'peer-helper': ('ffn', 11164, 'd7c6f3458d795f65635b80c3c035c93a7da5b325629f19e0b0fdecdc70fc629d', 'PeerHelper'),
}


def main(args):
    if len(args) != 3 or args[2] not in ('root-main', 'root-prefix', 'root-graph'):
        raise ValueError('expected ROOT PEER and one exact QFG2 root section')
    images = {}
    for kind, path in [('root', pathlib.Path(args[0])), ('peer', pathlib.Path(args[1]))]:
        if (path / 'boot.bin').read_bytes() != struct.pack('<I', 0x0006c06f):
            raise ValueError('exact shared boot mismatch')
        for name, (section, size, pin, _) in PINS.items():
            if not name.startswith(kind + '-'):
                continue
            data = (path / (section + '.bin')).read_bytes()
            if len(data) != size or len(data) % 4 or hashlib.sha256(data).hexdigest() != pin:
                raise ValueError('pinned QFG2/companion section mismatch: ' + name)
            images[name] = data
    name = args[2]
    _, size, pin, suffix = PINS[name]
    guard = 'PIONEER_Q38_FFN_INPUT_GRAPH_' + suffix.upper() + '_IMAGE_H'
    symbol = 'PioneerQ38FfnInputGraph' + suffix
    lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef ' + guard, '#define ' + guard,
             '#include <stdint.h>', '// Separate frozen QFG2 DATA,NOT a native selector/source/input/lifetime grant.',
             '// SHA256 ' + pin, 'static const uint32_t ' + symbol + '[]={']
    words = struct.unpack('<' + str(size // 4) + 'I', images[name])
    for at in range(0, len(words), 8):
        lines.append(' ' + ','.join('0x%08xU' % value for value in words[at:at+8]) + ',')
    lines += ['};', 'static_assert(sizeof(' + symbol + ')==' + str(size) + ',"exact pinned QFG2 section");', '#endif']
    print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/' + symbol + '.h')
    for line in lines:
        print('+' + line)
    print('*** End Patch')


if __name__ == '__main__':
    main(sys.argv[1:])
