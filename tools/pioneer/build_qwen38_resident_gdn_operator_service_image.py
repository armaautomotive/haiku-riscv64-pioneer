#!/usr/bin/env python3
"""OFFLINE persistent matrix/Conv/cooperative GDN IMAGE;NO device or native."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
sdk, output = map(pathlib.Path, sys.argv[1:])
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
output.mkdir(mode=0o700)
# Authenticate complete prior service AND its independently recompiled QFP3
# ancestor before deriving ANY compiler flags or building the new image.
saved = list(sys.argv)
try:
    sys.argv = [str(here/'build_qwen38_resident_operator_service_image.py'), str(sdk), str(output/'unchanged-matrix-conv')]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
for section, digest in (
    ('boot', 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 'ea6245c426a2707f76dc58cf329d838f40d4dd0b48ff4b4509fffc9b5122a1fb'),
    ('affine', '8c496193e749e8933e927f82c7785b097bc375f54f61971e4c7f454efc8bbaca'),
    ('ffn', 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', '1e015c2bea109f36a08da5a651e582d9d8aee81e5c4f29a7eda7eb614d702c92'),
):
    assert hashlib.sha256((output/'unchanged-matrix-conv'/(section+'.bin')).read_bytes()).hexdigest() == digest
qgt = output/'unchanged-qgt1'
subprocess.run(['sh', str(here/'build_qwen38_gdn_rendezvous_image.sh'), str(sdk), str(qgt)], check=True, env=clean)
assert hashlib.sha256((qgt/'code.bin').read_bytes()).hexdigest() == '3a99ac89d98236b7e3dac5e1cf433223961fdb1b7f0f6f80dd81d541a47b0284'
assert (qgt/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
cross, flags, peer = base['cross'], list(base['flags']), list(base['peer'])
roles = [*base['roles'], '-DPIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE=1']
forced = ['-include', str(here/'PioneerQ38ResidentGdnOperatorImageRole.h')]
units = [
    ('blackhole_qwen38_resident_operator_service.cpp', 'program', [*peer, *roles], '-O2'),
    ('blackhole_qwen38_ffn_persistent_activation.cpp', 'activation', ['-DPIONEER_Q38_PROJECTION_PERSISTENT=1'], '-O2'),
    ('blackhole_qwen38_resident_conv_service.cpp', 'conv', list(base['roles']), '-O2'),
    ('blackhole_qwen38_resident_gdn_service.cpp', 'gdn_service', [*roles, *forced], '-Os'),
    ('blackhole_qwen38_gdn_input.cpp', 'gdn_input', [*roles, *forced, '-DPIONEER_Q38_GDN_INPUT_COMBINED_STAGE=1', '-Dscalar_fault_park=block_fault_park'], '-O2'),
    ('blackhole_qwen38_resident_gdn.cpp', 'gdn_state', [*roles, *forced, '-DPIONEER_Q38_RGDN_COMBINED_STAGE=1', '-Dscalar_fault_park=block_fault_park'], '-O2'),
    ('blackhole_qwen38_gdn_recurrent.cpp', 'gdn_recurrent', [*roles, *forced, '-Dscalar_main=q38_resident_gdn_recurrent_stage', '-Dscalar_fault_park=block_fault_park'], '-Os'),
    ('blackhole_qwen38_gdn_gated_resident.cpp', 'gdn_gated', [*roles, *forced, '-DPIONEER_Q38_GDN_GATED_RESIDENT_IMAGE=1', '-DPIONEER_Q38_GDN_RENDEZVOUS_IMAGE=1', '-Dscalar_main=q38_resident_gdn_once', '-Dscalar_fault_park=block_fault_park'], '-Os'),
]
for source, name, extra, optimize in units:
    current = [arg for arg in flags if arg != '-O2']
    subprocess.run([str(cross)+'-g++', *current, optimize, *extra, '-c', str(here/source), '-o', str(output/(name+'.o'))], check=True, env=clean)
subprocess.run([str(cross)+'-as', '-march=rv32imf', '-mabi=ilp32', '-mno-relax', '--defsym', 'TT_Q38_FFN_PERSISTENT_PEER=1',
                str(here/'blackhole_qwen38_persistent_start.S'), '-o', str(output/'start.o')], check=True, env=clean)
subprocess.run([str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T',
                str(here/'blackhole_qwen38_resident_gdn_operator_service.ld'), str(output/'start.o'),
                *[str(output/(name+'.o')) for _, name, _, _ in units], '-Map', str(output/'program.map'), '-o', str(output/'program.elf')], check=True, env=clean)
for section in ('boot', 'text', 'affine', 'ffn', 'service', 'gdn'):
    subprocess.run([str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section, str(output/'program.elf'), str(output/(section+'.bin'))], check=True, env=clean)
for tool, argv, name in (
    ('nm', ['-n'], 'program.symbols'), ('nm', ['-u'], 'program.undefined'), ('objdump', ['-d'], 'program.disassembly'),
):
    with (output/name).open('w') as stream:
        subprocess.run([str(cross)+'-'+tool, *argv, str(output/'program.elf')], stdout=stream, check=True, env=clean)
assert not (output/'program.undefined').read_bytes()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (output/'program.symbols').read_text(), re.M)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s)\b', (output/'program.disassembly').read_text())
subprocess.run([sys.executable, str(here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py'), str(output/'program.disassembly')], check=True, env=clean)
previous = {}
for name in ('program', 'activation', 'conv'):
    previous.update(base['frames'](output/'unchanged-matrix-conv'/(name+'.su')))
for path in qgt.glob('*.su'):
    previous.update(base['frames'](path))
maximum = 0
for _, name, _, _ in units:
    for function, size in base['frames'](output/(name+'.su')).items():
        assert size <= 2048 or function in previous and size <= previous[function], (function, size, previous.get(function))
        if function not in previous:
            maximum = max(maximum, size)
assert (output/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for section in ('boot', 'text', 'affine', 'ffn', 'service', 'gdn'):
    data = (output/(section+'.bin')).read_bytes()
    print(hashlib.sha256(data).hexdigest()+' '+section+' bytes'+str(len(data)))
print('PASS OFFLINE returning GDN/matrix/Conv IMAGE compile/maxNewFrame'+str(maximum)+'/prior service+QFP3+standaloneQGT1 ALL EXACT;NEW GDN local mailbox74700 preserves persistent command74000')
print('Proposed GDN ROM b8000..c8000 and service68000..6e000 need NEW joint native footprint;NO native/device/ROOT/fullmodel or speed grant')
