#!/usr/bin/env python3
"""Audit two CPU-private original EOS-prefix controls; NEVER device proof."""
import hashlib
import os
import pathlib
import struct
import sys
import compare_pioneer_q38_eos_state as state
from compare_pioneer_q38_eos_trace import stable, require

PINS = ('9323e2451c41e241c9a0391d3905890b662fd73df8542103da229f9b910d6937',
        'dcc88c37599ab6659990f5154da04dcc12b4edea5a38c27359402b75ede4973d')


def audit(main, clean):
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    corpus = state.corpus()
    for directory in (main, clean):
        require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'CPU control directory')
        require({p.name for p in directory.iterdir()} == {'token-0.bin', 'token-1.bin', 'eos-state-cpu'}, 'CPU control exact two-token file set')
        for position in range(2):
            data = stable(directory/f'token-{position}.bin', 2324548)
            require(data[:8] == bytes.fromhex('5146523101000000') and data[-4:] == bytes.fromhex('51444e45'), 'CPU token family/footer')
            require(hashlib.sha256(data).hexdigest() == PINS[position], 'CPU token differs from pinned original three-step prefix')
        folder = directory/'eos-state-cpu'
        require(folder.is_dir() and not folder.is_symlink() and {p.name for p in folder.iterdir()} == set(corpus) | {'eos-state-complete.bin'}, 'CPU EOS state exact file set')
        marker = struct.pack('<16I', state.CPU_MAGIC, 1, 128, 2, 64, *([0]*11))+struct.pack('<I', state.END)
        require(stable(folder/'eos-state-complete.bin', len(marker)) == marker, 'CPU EOS state marker')
    words = 0
    for name, fields in corpus.items():
        first = state.read(main/'eos-state-cpu', name, state.CPU_MAGIC, fields)
        second = state.read(clean/'eos-state-cpu', name, state.CPU_MAGIC, fields)
        require(first == second, f'CPU state controls BIT MISMATCH {name};NO tolerance or repair')
        words += fields[2]
    for position, digest in enumerate(PINS):
        print(f'CPU_EOS_PREFIX position{position}/bytes2324548/SHA256{digest}/UNCHANGED from pinned three-step original control')
    print(f'PASS two CPU-private original controls/128 records/{words} words BIT-IDENTICAL/exact2 tokens+marker/finite FP32+packedhalf domains/CPU QCE1 distinct from observed QNE1;SAME CPU algorithm run twice,NOT accelerator comparison,EOS execution,END,Work or TPS qualification')


if __name__ == '__main__':
    require(len(sys.argv) == 3, 'expected two CPU control directories')
    audit(*map(pathlib.Path, sys.argv[1:]))
