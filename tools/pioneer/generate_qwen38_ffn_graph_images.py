#!/usr/bin/env python3
"""Print one apply_patch addition for a pinned private FFN code section.

This freezes DATA only. It never writes files,builds,opens SDKs or registers
a native selector. All ROOT+PEER sections must verify before ANY patch output.
"""
import hashlib
import pathlib
import struct
import sys

PINS = {
    'root-main': ('text', 6884, '19eba2d3fb8f0d4cee5debd4bd4960856153ba7592c4892ffecacdb250b23494', 'RootMain'),
    'root-prefix': ('prefix', 6148, '3f7f3b0b42e0fbf6b002eb58cadbc954ed195a3108040d73770446af63dc19e1', 'RootPrefix'),
    'root-graph': ('graph', 16464, '9dfca41e5dd72117a98cf79cc0c1dc2ad33b5bfebc3b3b3c70f1f70bd0a9000c', 'RootGraph'),
    'peer-main': ('text', 8180, 'd2052fbbf43dcc022747ea2b228e7b1bc0f829f4984821ab4dfca20473e3a8ef', 'PeerMain'),
    'peer-affine': ('affine', 5152, '96e92065722a477b08ba41abc2f623064d44970c5807b37c6d739e8dc3697570', 'PeerAffine'),
    'peer-helper': ('ffn', 11164, 'd7c6f3458d795f65635b80c3c035c93a7da5b325629f19e0b0fdecdc70fc629d', 'PeerHelper'),
}


def main(args):
    if len(args) != 3 or args[2] not in PINS:
        raise ValueError('expected ROOT PEER and one exact private section name')
    images = {}
    for kind, path in [('root', pathlib.Path(args[0])), ('peer', pathlib.Path(args[1]))]:
        boot = (path / 'boot.bin').read_bytes()
        if boot != struct.pack('<I', 0x0006c06f):
            raise ValueError('exact common boot mismatch')
        for name, (section, size, pin, _) in PINS.items():
            if not name.startswith(kind + '-'):
                continue
            data = (path / (section + '.bin')).read_bytes()
            if len(data) != size or len(data) % 4 or hashlib.sha256(data).hexdigest() != pin:
                raise ValueError('pinned private section mismatch: ' + name)
            images[name] = data
    name = args[2]
    _, size, pin, suffix = PINS[name]
    guard = 'PIONEER_Q38_FFN_GRAPH_' + suffix.upper() + '_IMAGE_H'
    lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef ' + guard, '#define ' + guard,
             '#include <stdint.h>', '// Frozen private FFN code DATA,NOT a native image selector or launch grant.',
             '// SHA256 ' + pin, 'static const uint32_t PioneerQ38FfnGraph' + suffix + '[]={']
    words = struct.unpack('<' + str(size // 4) + 'I', images[name])
    for at in range(0, len(words), 8):
        lines.append(' ' + ','.join('0x%08xU' % value for value in words[at:at+8]) + ',')
    lines += ['};', 'static_assert(sizeof(PioneerQ38FfnGraph' + suffix + ')==' + str(size) + ',"exact pinned FFN section");', '#endif']
    print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraph' + suffix + '.h')
    for line in lines:
        print('+' + line)
    print('*** End Patch')


if __name__ == '__main__':
    main(sys.argv[1:])
