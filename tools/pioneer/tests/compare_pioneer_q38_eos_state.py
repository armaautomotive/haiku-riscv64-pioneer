#!/usr/bin/env python3
"""Posthoc two-position state DATA comparison; NEVER execution/END authority."""
import pathlib
import struct
import sys
from compare_pioneer_q38_eos_trace import stable, words, require

CPU_MAGIC, OBSERVED_MAGIC, END = 0x31454351, 0x31454e51, 0x444e4553


def corpus():
    result = {}
    for layer in range(64):
        for kind in ((2, 3) if layer % 4 == 3 else (0, 1)):
            count = 786432 if kind == 0 else 30720 if kind == 1 else 1024
            result[f'eos-c2-l{layer}-k{kind}.bin'] = (layer, kind, count)
    require(len(result) == 128, 'EOS state schema')
    return result


def read(directory, name, magic, fields):
    layer, kind, count = fields
    data = stable(directory/name, (16+count+1)*4)
    require(struct.unpack_from('<16I', data) == (magic, 1, 2, layer, kind, count, *([0]*10)), 'EOS state family/identity/header')
    require(struct.unpack_from('<I', data, len(data)-4)[0] == END, 'EOS state footer')
    values = words(data[64:-4])
    if kind < 2:
        require(all(w & 0x7f800000 != 0x7f800000 for w in values), 'EOS nonfinite FP32 state')
    else:
        require(all(w & 0x7c00 != 0x7c00 and w & 0x7c000000 != 0x7c000000 for w in values), 'EOS nonfinite packed binary16 cache')
    return data[64:-4]


def compare(cpu, observed):
    expected = corpus()
    for directory, magic in ((cpu, CPU_MAGIC), (observed, OBSERVED_MAGIC)):
        require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'EOS state directory')
        require({p.name for p in directory.iterdir()} == set(expected) | {'eos-state-complete.bin'}, 'EOS state exact file set')
        marker = struct.pack('<16I', magic, 1, 128, 2, 64, *([0]*11)) + struct.pack('<I', END)
        require(stable(directory/'eos-state-complete.bin', len(marker)) == marker, 'EOS state incomplete marker')
    total = 0
    for name, fields in expected.items():
        original = read(cpu, name, CPU_MAGIC, fields)
        actual = read(observed, name, OBSERVED_MAGIC, fields)
        require(actual == original, f'EOS state BIT MISMATCH {name};NO tolerance/repair/diagnostic-only pass')
        total += fields[2]
    print(f'PASS EOS128 state DATA records/{total} words BIT-EXACT/all48 GDN+Conv and16 packed K+V at EXACTLY2 positions;separate ORIGINAL source/execution/EOS/STOP/END proof REQUIRED;NOT hardware,Work or TPS qualification')


if __name__ == '__main__':
    require(len(sys.argv) == 3, 'expected CPU and observed EOS state directories')
    try:
        compare(*map(pathlib.Path, sys.argv[1:]))
    except (ValueError, OSError) as error:
        print(f'EOS_STATE_REFUSAL {error}', file=sys.stderr)
        sys.exit(2)
