#!/usr/bin/env python3
"""Print apply_patch additions for EXACT offline QFG3 root sections.

ALL root and unchanged peer sections authenticate before any patch output.
Frozen DATA only;no native selection,ownership,build or hardware access.
"""
import hashlib
import pathlib
import struct
import sys

ROOT = {
    'main': ('text', 6884, '492a94d2435a4794c759c2eb3b9b50de29d40c1ad53cf9819ce1e68f73ab5ac5', 'Main'),
    'prefix': ('prefix', 6172, '28d8513c2bcdd5f7a0eb2b4ff4870715429611dacf319aef5952167f7a5dc9ce', 'Prefix'),
    'graph': ('graph', 17824, '361fa896b9874c99686de9de577d9990fd0e7cd97692591ec36b71a242358de1', 'Graph'),
}
PEER = {
    'text': (8180, 'd2052fbbf43dcc022747ea2b228e7b1bc0f829f4984821ab4dfca20473e3a8ef'),
    'affine': (5152, '96e92065722a477b08ba41abc2f623064d44970c5807b37c6d739e8dc3697570'),
    'ffn': (11164, 'd7c6f3458d795f65635b80c3c035c93a7da5b325629f19e0b0fdecdc70fc629d'),
}


def main(args):
    if len(args) != 3 or args[2] not in ROOT:
        raise ValueError('expected QFG3_ROOT QFP2_PEER and main/prefix/graph')
    root, peer = map(pathlib.Path, args[:2])
    for image in (root, peer):
        if (image/'boot.bin').read_bytes() != struct.pack('<I', 0x0006c06f):
            raise ValueError('fixed boot mismatch')
    images = {}
    for name, (section, size, digest, _) in ROOT.items():
        data = (root/(section+'.bin')).read_bytes()
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('QFG3 section mismatch: '+name)
        images[name] = data
    for section, (size, digest) in PEER.items():
        data = (peer/(section+'.bin')).read_bytes()
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('unchanged QFP2 section mismatch: '+section)
    _, size, digest, suffix = ROOT[args[2]]
    symbol = 'PioneerQ38FfnResidualRoot'+suffix
    guard = 'PIONEER_Q38_FFN_RESIDUAL_ROOT_'+suffix.upper()+'_H'
    lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef '+guard, '#define '+guard,
             '#include <stdint.h>', '// Frozen offline QFG3 DATA,not a native mapping/launch/producer grant.',
             '// SHA256 '+digest, 'static const uint32_t '+symbol+'[]={']
    words = struct.unpack('<'+str(size//4)+'I', images[args[2]])
    for at in range(0, len(words), 8):
        lines.append(' '+','.join('0x%08xU' % v for v in words[at:at+8])+',')
    lines += ['};', 'static_assert(sizeof('+symbol+')=='+str(size)+',"exact pinned QFG3 section");', '#endif']
    print('*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+symbol+'.h')
    for line in lines:
        print('+'+line)
    print('*** End Patch')


if __name__ == '__main__':
    main(sys.argv[1:])
