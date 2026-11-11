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
reuse_head = len(sys.argv) == 4 and sys.argv[3] == '--head-bfp8-reuse-inspection'
batch_dense = len(sys.argv) == 4 and sys.argv[3] == '--dense-q8-srca-batch-inspection'
batch_head = batch_dense or (len(sys.argv) == 4 and sys.argv[3] == '--head-bfp8-batch-inspection')
combined = batch_dense or (len(sys.argv) == 4 and sys.argv[3] == '--dense-q8-srca-inspection')
dense_q8 = combined or (len(sys.argv) == 4 and sys.argv[3] == '--dense-q8-matrix-inspection')
srca_base = combined or (len(sys.argv) == 4 and sys.argv[3] == '--head-srca-base-inspection')
head_matrix = len(sys.argv) == 4 and sys.argv[3] in ('--head-matrix-inspection', '--head-bfp8-reuse-inspection', '--head-bfp8-batch-inspection', '--dense-q8-matrix-inspection', '--head-srca-base-inspection', '--dense-q8-srca-inspection', '--dense-q8-srca-batch-inspection')
assert len(sys.argv) == 3 or test_layer0 or head_matrix
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
if head_matrix:
    global_role.append('-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1')
if dense_q8:
    global_role.append('-DPIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION=1')
if srca_base:
    global_role.append('-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE=1')
if combined:
    global_role.append('-DPIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER=1')
if reuse_head:
    global_role.append('-DPIONEER_Q38_QGW8_BFP8_REUSE_WORKER=1')
if batch_head:
    global_role.append('-DPIONEER_Q38_QGW8_BFP8_BATCH_WORKER=1')
if batch_dense:
    global_role.append('-DPIONEER_Q38_QGWE_BATCH_DENSE_WORKER=1')
run(tool+'g++', *flags, '-Os', '-ffixed-s11', *global_role, *inspection, '-c',
    here/'blackhole_qwen38_global_worker.cpp', '-o', out/'global.o')
if reuse_head or batch_head:
    run(tool+'g++', *flags, '-O2', '-fno-builtin', '-ffixed-s11', *global_role, *inspection,
        '-DPIONEER_Q38_BFP8_INPUT_REUSE_EXPERIMENT=1' if reuse_head else '-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1', '-Dnative_dram_band_fault=block_fault_park',
        '-Dnative_dram_band_main=q38_bfp8_worker_leaf', '-c',
        here/'blackhole_q38_bfp8_worker_leaf.cpp', '-o', out/'bfp8.o')
if dense_q8:
    run(tool+'g++', *flags, '-O2', '-ffixed-s11', *global_role, *inspection, '-c',
        here/'blackhole_q38_bf16_persistent_producer.cpp', '-o', out/'bf16.o')
dense_role = ['-DPIONEER_Q38_MIXED_WORKER_IMAGE=1', '-DPIONEER_Q38_MIXED_DENSE_CAPSULE_IMAGE=1',
              '-DPIONEER_Q38_MIXED_CAPSULE_IMAGE=1', '-Dscalar_fault_park=block_fault_park',
              '-include', here/'PioneerQ38MixedCapsuleHook.h']
if srca_base:
    dense_role += ['-DPIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE=1','-DPIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION=1']
run(tool+'g++', *flags, '-O2', '-ffixed-s11', *dense_role, *inspection, '-c',
    here/'blackhole_qwen38_dense_partition.cpp', '-o', out/'dense.o')
parent = base['baseline']['parent']
old_math = out/'baseline/baseline/unchanged-global/qualified-math'
units = [name for name in parent['units'] if name != 'dense']
assert len(units) + 1 == len(parent['units'])
link = (out/'baseline/program.ld').read_text()
(out/'program.ld').write_text(link)
run(tool+'ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', out/'program.ld',
    out/'baseline/baseline/start.o', out/'global.o', out/('bf16.o' if dense_q8 else 'baseline/baseline/bf16.o'),
    out/'baseline/bfp4.o', out/('bfp8.o' if reuse_head or batch_head else 'baseline/bfp8.o'), out/'dense.o',
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
sections_to_audit = ('.globalworker', '.dense') + (('.cachedfpu',) if reuse_head or batch_head else ()) + (('.bf16producer',) if dense_q8 else ())
for section in sections_to_audit:
    section_dis = subprocess.check_output([tool+'objdump', '-d', '-j', section, str(out/'program.elf')], text=True)
    assert not re.search(r'\bsb\b', section_dis), section
    assert not re.search(r'<(?:memcpy|memset)>', section_dis), section+' may not call the old byte-store fallbacks'
reserved = [row for row in dis.splitlines() if re.search(r'\bs11\b', row)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', row)) for row in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', row)) for row in reserved) == 1, 'exact existing private-seal protocol'
for name in (('global', 'dense', 'bf16') if dense_q8 else ('global', 'dense')):
    frames = [row.split('\t') for row in (out/(name+'.su')).read_text().splitlines()]
    assert frames and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 2048 for row in frames)
(out/'program.symbols').write_text(symbols); (out/'program.disassembly').write_text(dis)
sections = ('boot','globalworker','bf16producer','cachedfpu','dense','text','affine','ffn','service','gdn')
for section in sections:
    path = out/(section+'.bin'); run(tool+'objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', path)
    data = path.read_bytes(); assert data and not len(data)%4
    if section == 'dense': assert len(data) <= 16384, 'dense image exceeds captured 0x40000..0x44000'
    if section not in ('globalworker', 'dense') and not ((reuse_head or batch_head) and section == 'cachedfpu') and not (dense_q8 and section == 'bf16producer'):
        # The inputs are the original objects, but changed global function
        # addresses legitimately alter link-time call relocations in their
        # output sections. Require their extents and the boot jump unchanged;
        # do not claim that every linked byte is identical.
        old_data = (out/'baseline'/(section+'.bin')).read_bytes()
        assert len(data) == len(old_data), section
        if section == 'boot': assert data == old_data
    print('QGWE_BUILD' if batch_dense else 'QGWD_BUILD' if combined else 'QGWC_BUILD' if srca_base else 'QGWB_BUILD' if dense_q8 else 'QGWA_BUILD' if batch_head else 'QGW9_BUILD' if reuse_head else 'QGW8_BUILD' if head_matrix else 'QGW7_BUILD', section, len(data), hashlib.sha256(data).hexdigest(), flush=True)
print('BUILD ONLY QGWE batched dense-Q8 worker: NO ROOT/native owner/model/hardware/quality/speed authority' if batch_dense
      else 'BUILD ONLY QGWD SrcA-corrected dense-Q8 worker: NO ROOT/native owner/model/hardware/quality/speed authority' if combined
      else 'BUILD ONLY QGWC SrcA base hypothesis worker: NO ROOT/native owner/model/hardware/quality/speed authority' if srca_base
      else 'BUILD ONLY QGWB dense Q8 matrix worker inspection: NO ROOT/native owner/model/hardware/quality/speed authority' if dense_q8
      else 'BUILD ONLY QGWA batched-input worker inspection: NO ROOT/native owner/model/hardware/quality/speed authority' if batch_head
      else 'BUILD ONLY QGW9 retained-input worker inspection: NO ROOT/native owner/model/hardware/quality/speed authority' if reuse_head
      else 'BUILD ONLY QGW8 head-matrix worker inspection: NO ROOT/native owner/model/hardware/quality/speed authority' if head_matrix
      else 'BUILD ONLY QGW7/QDM2 inspection: NO ROOT/native owner/model/hardware/quality/speed authority')
if test_layer0:
    print('TEST ONLY BFP8 layer-zero mask; not a real-model worker')
if reuse_head:
    print('QGW9 REUSE WORKER INSPECTION ONLY: no ROOT/native/ownership/device/Work authority')
