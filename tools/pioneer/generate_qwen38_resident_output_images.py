#!/usr/bin/env python3
"""Emit one apply_patch for complete pinned code DATA; NEVER install/launch."""
import hashlib
import pathlib
import struct
import sys

PINS = {
    'root-boot': ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b', 'RootBoot'),
    'root-main': ('text', 92, 'a5c7dc3a5349fddf0f27570c6a4ae1b22ad7b60ee418931235404eaf7a141aae', 'RootMain'),
    'root-conv': ('conv', 6472, '6ed515323e4b3376f6e3f4bc8d78348643a6c758d822170fee799cec404a4694', 'RootConv'),
    'root-matrix': ('matrix', 14136, 'e86c2813892c807d6fe51c5f65fe58199169ff189d4abea15e2c96b89f4d9f6a', 'RootMatrix'),
    'root-cohort': ('cohort', 50692, '76958ca27a42af608d44fc6d3e9f4561853a61057b4d1b9889dc1acd0198bc7d', 'RootCohort'),
    'peer-boot': ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b', 'PeerBoot'),
    'peer-main': ('text', 7940, 'd726337fe94d7d94812aaf5e67f3d4e328a21873cb27aa982dc5315bb6294b07', 'PeerMain'),
    'peer-affine': ('affine', 5360, 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b', 'PeerAffine'),
    'peer-ffn': ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9', 'PeerFfn'),
    'peer-service': ('service', 10300, 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254', 'PeerService'),
    'peer-gdn': ('gdn', 35396, '36458af185c3500d1cd1b1170672c2138910aff836bd8a6a407b07c68a80b93d', 'PeerGdn'),
}


def generate(args):
    if len(args) != 3 or args[2] not in PINS:
        raise ValueError('expected ROOT PEER and one exact complete resident-output section')
    images = {}
    # ALL eleven sections and BOTH boots authenticate before ANY output,
    # including when the requested leaf happens to be a tiny boot/main.
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
    name = args[2]
    _, size, digest, suffix = PINS[name]
    symbol = 'PioneerQ38ResidentOutput'+suffix
    guard = 'PIONEER_Q38_RESIDENT_OUTPUT_'+suffix.upper()+'_IMAGE_H'
    lines = ['/* SPDX-License-Identifier: MIT */', '#ifndef '+guard, '#define '+guard,
             '#include <stdint.h>', '// Exact complete frozen code DATA,NOT native/model/source/init/owner authority.',
             '// SHA256 '+digest, 'static const uint32_t '+symbol+'[]={']
    words = struct.unpack('<'+'I'*(size//4), images[name])
    for i in range(0, len(words), 8):
        lines.append(' '+','.join('0x%08xU' % value for value in words[i:i+8])+',')
    lines += ['};', 'static_assert(sizeof('+symbol+')=='+str(size)+',"exact complete resident-output code DATA");', '#endif']
    return '*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+symbol+'.h\n'+''.join('+'+line+'\n' for line in lines)+'*** End Patch\n'


if __name__ == '__main__':
    try:
        output = generate(sys.argv[1:])
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
    print(output, end='')
