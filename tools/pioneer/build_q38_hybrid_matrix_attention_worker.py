#!/usr/bin/env python3
"""Build isolated QGW7 hybrid + QDM2 dense worker. Never installs or uses a card."""
import hashlib
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

assert __debug__ and len(sys.argv) in (3, 4) and not os.environ.get('PYTHONOPTIMIZE')
test_layer0 = len(sys.argv) == 4 and sys.argv[3] == '--test-bfp8-layer0'
assert len(sys.argv) == 3 or test_layer0
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(Path, sys.argv[1:3]); here = Path(__file__).resolve().parent
assert sdk.is_absolute() and sdk.is_dir() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_q38_hybrid_worker.py'), str(sdk), str(out/'baseline')]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
sys.path.insert(0, str(here))
from PioneerQ38HybridProgramPins import WORKER_PINS
for name, size, digest in WORKER_PINS:
    data = (out/'baseline'/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
tool, flags = base['tool'], base['flags']
def run(*args):
    subprocess.run([str(a) for a in args], check=True)

inspection = ['-DPIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION=1',
              '-DPIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT=1']
global_role = ['-DPIONEER_Q38_BF16_WORKER_IMAGE=1', '-DPIONEER_Q38_HYBRID_WORKER_IMAGE=1',
               '-DPIONEER_DRAM_SWEEP_FREESTANDING=1']
if test_layer0:
    global_role.append('-DPIONEER_Q38_HYBRID_TEST_BFP8_LAYER0=1')
run(tool+'g++', *flags, '-Os', '-ffixed-s11', *global_role, *inspection, '-c',
    here/'blackhole_qwen38_global_worker.cpp', '-o', out/'global.o')
dense_role = ['-DPIONEER_Q38_MIXED_WORKER_IMAGE=1', '-DPIONEER_Q38_MIXED_DENSE_CAPSULE_IMAGE=1',
              '-DPIONEER_Q38_MIXED_CAPSULE_IMAGE=1', '-Dscalar_fault_park=block_fault_park',
              '-include', here/'PioneerQ38MixedCapsuleHook.h']
run(tool+'g++', *flags, '-O2', '-ffixed-s11', *dense_role, *inspection, '-c',
    here/'blackhole_qwen38_dense_partition.cpp', '-o', out/'dense.o')
parent = base['baseline']['parent']
old_math = out/'baseline/baseline/unchanged-global/qualified-math'
units = [name for name in parent['units'] if name != 'dense']
assert len(units) + 1 == len(parent['units'])
link = (out/'baseline/program.ld').read_text()
(out/'program.ld').write_text(link)
run(tool+'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', out/'program.ld',
    out/'baseline/baseline/start.o', out/'global.o', out/'baseline/baseline/bf16.o',
    out/'baseline/bfp4.o', out/'baseline/bfp8.o', out/'dense.o',
    *[old_math/(name+'.o') for name in units], '-Map', out/'program.map', '-o', out/'program.elf')
symbols = subprocess.check_output([tool+'nm', '-n', str(out/'program.elf')], text=True)
assert not subprocess.check_output([tool+'nm', '-u', str(out/'program.elf')]).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name in ('q38_global_worker_main', 'q38_mixed_seal', 'q38_mixed_dense_phase',
             'q38_bfp4_worker_leaf', 'q38_bfp8_worker_leaf'):
    assert re.search(r' T '+name+'$', symbols, re.M), name
dis = subprocess.check_output([tool+'objdump', '-d', str(out/'program.elf')], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
# The byte-identical ancestral GDN object includes byte-copy fallback paths;
# the new front/dense sections must add no byte-store instructions. This is
# not a reachability or hardware-ISA proof for the old fallback paths.
for section in ('.globalworker', '.dense'):
    section_dis = subprocess.check_output([tool+'objdump', '-d', '-j', section, str(out/'program.elf')], text=True)
    assert not re.search(r'\bsb\b', section_dis), section
    assert not re.search(r'<(?:memcpy|memset)>', section_dis), section+' may not call the old byte-store fallbacks'
reserved = [row for row in dis.splitlines() if re.search(r'\bs11\b', row)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', row)) for row in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', row)) for row in reserved) == 1, 'exact existing private-seal protocol'
for name in ('global', 'dense'):
    frames = [row.split('\t') for row in (out/(name+'.su')).read_text().splitlines()]
    assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 2048 for row in frames)
(out/'program.symbols').write_text(symbols); (out/'program.disassembly').write_text(dis)
sections = ('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn')
for section in sections:
    path = out/(section+'.bin'); run(tool+'objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', path)
    data = path.read_bytes(); assert data and not len(data)%4
    if section == 'dense': assert len(data) <= 16384, 'dense image exceeds captured 0x40000..0x44000'
    if section not in ('globalworker', 'dense'):
        # The inputs are the original objects, but changed global function
        # addresses legitimately alter link-time call relocations in their
        # output sections. Require their extents and the boot jump unchanged;
        # do not claim that every linked byte is identical.
        old_data = (out/'baseline'/(section+'.bin')).read_bytes()
        assert len(data) == len(old_data), section
        if section == 'boot': assert data == old_data
    print('QGW7_BUILD', section, len(data), hashlib.sha256(data).hexdigest(), flush=True)
print('BUILD ONLY QGW7/QDM2 inspection: NO ROOT/native owner/model/hardware/quality/speed authority')
if test_layer0:
    print('TEST ONLY BFP8 layer-zero mask; not a real-model worker')
