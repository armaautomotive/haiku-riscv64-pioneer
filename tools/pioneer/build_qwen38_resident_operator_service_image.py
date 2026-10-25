#!/usr/bin/env python3
"""OFFLINE persistent matrix+Conv service image;NO native install/device."""
import hashlib
import os
import pathlib
import re
import runpy
import shlex
import struct
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
sdk, output = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
cross = sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
assert pathlib.Path(str(cross)+'-g++').is_file()
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
output.mkdir(mode=0o700)
old = output/'unchanged-qfp3'
subprocess.run(['sh', str(here/'build_qwen38_ffn_persistent_image.sh'), str(sdk), str(old), '--projection-input'],
               check=True, env={**clean, 'PIONEER_Q38_FFN_PERSISTENT': '1'})
assert (old/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for name, (section, size, digest, _) in runpy.run_path(str(here/'generate_qwen38_projection_graph_images.py'))['PINS'].items():
    if name.startswith('peer-'):
        data = (old/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, 'old QFP3 MUST remain byte-exact'
flags = shlex.split('''-std=c++11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-exceptions -fno-rtti
 -fno-stack-protector -fno-pic -fno-pie -ffp-contract=off -ffunction-sections -fdata-sections
 -fno-unwind-tables -fno-asynchronous-unwind-tables -msmall-data-limit=0 -mno-relax
 -march=rv32imf -mabi=ilp32 -fstack-usage''')
peer = shlex.split('''-DPIONEER_Q38_FFN_PERSISTENT -DPIONEER_Q38_FAST_PANEL -DPIONEER_Q38_FAST_SCALE
 -DPIONEER_Q38_Q4_TWO_PHASE -DPIONEER_Q38_PIPELINED_MATRIX -DPIONEER_Q38_STATIC_VECTOR
 -DPIONEER_Q38_CACHED_AFFINE -DPIONEER_Q38_REPLAY_OPERAND -DPIONEER_Q38_WORK_SKIP_DOT_CHECK
 -DPIONEER_Q38_QUEUED_MATRIX_ENTRY -DPIONEER_Q38_WORK_INPUT_SUM_CACHE -DPIONEER_Q38_WORK_ZERO_MINIMUM
 -DPIONEER_Q38_NATIVE_OPERAND_STORE -DPIONEER_Q38_WORK_DEFER_AFFINE -DPIONEER_Q38_INLINE_CONFIG
 -DPIONEER_Q38_BLOCK_AFFINE_DST -DPIONEER_Q38_BLOCK_SCALE_REUSE -DPIONEER_Q38_WORK_AFFINE_REPLAY
 -DPIONEER_Q38_WORK_DIRECT_SRCB -DPIONEER_Q38_WORK_DOT_ZERO_ELISION''')
roles = ['-DPIONEER_Q38_RESIDENT_OPERATOR_IMAGE=1', '-DPIONEER_Q38_FFN_PERSISTENT=1', '-DPIONEER_Q38_PROJECTION_PERSISTENT=1']
# Avoid a contradictory duplicate bare FFN definition on the program TU.
peer.remove('-DPIONEER_Q38_FFN_PERSISTENT')
for source, obj, extra in (
    ('blackhole_qwen38_resident_operator_service.cpp', 'program', [*peer, *roles]),
    ('blackhole_qwen38_ffn_persistent_activation.cpp', 'activation', ['-DPIONEER_Q38_PROJECTION_PERSISTENT=1']),
    ('blackhole_qwen38_resident_conv_service.cpp', 'conv', roles),
):
    subprocess.run([str(cross)+'-g++', *flags, *extra, '-c', str(here/source), '-o', str(output/(obj+'.o'))], check=True, env=clean)
subprocess.run([str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax', '--defsym', 'TT_Q38_FFN_PERSISTENT_PEER=1',
                str(here/'blackhole_qwen38_persistent_start.S'), '-o', str(output/'start.o')], check=True, env=clean)
subprocess.run([str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
                str(here/'blackhole_qwen38_resident_operator_service.ld'), *[str(output/(name+'.o')) for name in ('start', 'program', 'activation', 'conv')],
                '-Map', str(output/'program.map'), '-o', str(output/'program.elf')], check=True, env=clean)
for section in ('boot', 'text', 'affine', 'ffn', 'service'):
    subprocess.run([str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section, str(output/'program.elf'), str(output/(section+'.bin'))], check=True, env=clean)
for tool, argv, path in (
    ('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'), ('objdump', ['-d'], 'program.disassembly'),
):
    with (output/path).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *argv, str(output/'program.elf')], stdout=stream, check=True, env=clean)
assert not (output/'program.undefined').read_bytes()
symbols = (output/'program.symbols').read_text()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
disassembly = (output/'program.disassembly').read_text()
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub)\b', disassembly)
def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        row = line.split('\t');assert len(row) == 3 and row[2] == 'static'
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', row[0]);assert match
        key = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        result[key] = max(result.get(key, 0), int(row[1]))
    assert result
    return result


previous = {}
for name in ('program', 'activation'):
    previous.update(frames(old/(name+'.su')))
maximum, inherited = 0, 0
for name in ('program', 'activation', 'conv'):
    for function, size in frames(output/(name+'.su')).items():
        assert size <= 2048 or function in previous and size <= previous[function], (function, size, previous.get(function))
        if function in previous:
            inherited = max(inherited, size)
        else:
            maximum = max(maximum, size)
assert (output/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for section in ('boot', 'text', 'affine', 'ffn', 'service'):
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+'  '+str(output/(section+'.bin'))+' bytes'+str(len(data)))
print('PASS OFFLINE resident operator service compile/maxNewFunctionFrame'+str(maximum)+'/inheritedNongrowingMax'+str(inherited)+'/old QFP3 EXACT;SAME persistent sequence/matrix+op7 precise Conv;NO CPU ref/host callback/reset between operations')
print('NEW68000..6e000 service is NOT in existing ALL119 native snapshots;NO native selector/install/device/full model or speed qualification')
