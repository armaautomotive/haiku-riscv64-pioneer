#!/usr/bin/env python3
"""Emit apply_patch for exact NEW full-layer code DATA;NEVER install/launch."""
import hashlib
import pathlib
import struct
import sys

PINS = {
    'root-boot': ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b', 'RootBoot'),
    'root-main': ('text', 128, '98a89f90feb6fdd4bf6a072db9ef38064a6089d99d63b09d38d6af1c4ae495d3', 'RootMain'),
    'root-conv': ('conv', 6472, '479cb028a09ff252c8df32b279facfbf295d1b7cd72a13d2966272406398da51', 'RootConv'),
    'root-matrix': ('matrix', 14136, '1d2e2e7e4c8f806ff2983e5d9a2ccbd1a4d177940c7e23d73b6de33bae7e7559', 'RootMatrix'),
    'root-cohort': ('cohort', 59288, 'fca3da36bc93a1976c2f4609b423c8023a51661f58cc8267848f2e7bbe55c0a7', 'RootCohort'),
    'root-norm': ('norm', 5404, '60f44ff5f83914fedcaee02d70dd0c1a821edfc4a150e4406430d82d30dbeef1', 'RootNorm'),
    'root-ffn': ('ffn', 30496, 'e0c5de18d44c1ecce93b9e3282e853de1943da2a4ffae7c6ef482d0493ad9f3f', 'RootFfn'),
    'peer-boot': ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b', 'PeerBoot'),
    'peer-main': ('text', 7940, 'd726337fe94d7d94812aaf5e67f3d4e328a21873cb27aa982dc5315bb6294b07', 'PeerMain'),
    'peer-affine': ('affine', 5360, 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b', 'PeerAffine'),
    'peer-ffn': ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9', 'PeerFfn'),
    'peer-service': ('service', 10300, 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254', 'PeerService'),
    'peer-gdn': ('gdn', 35396, '36458af185c3500d1cd1b1170672c2138910aff836bd8a6a407b07c68a80b93d', 'PeerGdn'),
}


def generate(args):
    if len(args) != 3 or args[2] not in PINS or not args[2].startswith('root-'):
        raise ValueError('expected ROOT PEER and one exact NEW complete layer ROOT section')
    images = {}
    # Authenticate ALL13 complete sections before ANY output,even when
    # requesting only a tiny boot/main. Identical old peer constants are
    # reused as DATA ONLY,never as an old native ownership/launch receipt.
    for family, path in (('root', pathlib.Path(args[0])), ('peer', pathlib.Path(args[1]))):
        if (path/'boot.bin').read_bytes() != struct.pack('<I', 0x0006c06f):
            raise ValueError('complete exact boot mismatch: '+family)
        for name, (section, size, digest, _) in PINS.items():
            if not name.startswith(family+'-'):
                continue
            data = (path/(section+'.bin')).read_bytes()
            if len(data) != size or len(data) % 4 or hashlib.sha256(data).hexdigest() != digest:
                raise ValueError('complete pinned section mismatch: '+name)
            images[name] = data
    _, size, digest, suffix = PINS[args[2]]
    symbol = 'PioneerQ38ResidentLayer'+suffix
    guard = 'PIONEER_Q38_RESIDENT_LAYER_'+suffix.upper()+'_IMAGE_H'
    lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef '+guard, '#define '+guard,
             '#include <stdint.h>', '// Exact NEW complete code DATA,NOT native/model/source/init/owner authority.',
             '// SHA256 '+digest, 'static const uint32_t '+symbol+'[]={']
    words = struct.unpack('<'+'I'*(size//4), images[args[2]])
    for i in range(0, len(words), 8):
        lines.append(' '+','.join('0x%08xU' % value for value in words[i:i+8])+',')
    lines += ['};', 'static_assert(sizeof('+symbol+')=='+str(size)+',"exact NEW complete layer code DATA");', '#endif']
    return '*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+symbol+'.h\n'+''.join('+'+line+'\n' for line in lines)+'*** End Patch\n'


if __name__ == '__main__':
    try:
        result = generate(sys.argv[1:])
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
    print(result, end='')
