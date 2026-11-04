#!/usr/bin/env python3
"""Build a distinct complete vector-metadata worker;NEVER install/card IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent/'tests'))
from pioneer_q38_vector_q4_relocations import audit

PINS = (
    ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin',9232,'fda50f122034fde534eb859f387ce3093c6f839a6adfcc43eb0f59617e226bd1'),
    ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin',1292,'50a4910cc7d684e20c456f63f87478f520d639ec0ecaacdef06d50611dd81f29'),
    ('text.bin',8136,'68aad877f262785d7aa605d4fe02671011e04d4feab9151752111434c5e1cc91'),
    ('affine.bin',4108,'faf446d93d4e289de7870db0eb36b89ed4c59831ef40a46ea5a5a64ae8fe47c1'),
    ('ffn.bin',16456,'b2a1aec0b0d183975a3bdd361bab37e4834ed161c8757535251fdbcdd386b2a8'),
    ('service.bin',9888,'a01999b8d82275cb9195d51df013bb3563fffc83fab4609785fc30bfbbecf3b0'),
    ('gdn.bin',35944,'9ac7c07a748d5f5758ced6397053e5808bb94ba514a268d55471bdd7ebe4397f'),
    ('program.elf',144592,'4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870'),
)  # Independent main/clean BUILD identity only,not native authority.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or PINS)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_qwen38_wide_gdn_worker_image.py'), str(sdk), str(out/'unchanged-wide')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
control = out/'unchanged-wide'
tool = parent['tool']
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest() == '70fdf96d8edbc38d30391a5f180d5839e97581c9fb8e03f724c74be014e3e028'
command = (control/'unchanged-twin/compile-math-argv.txt').read_text().splitlines()
original = pathlib.Path(command[command.index('-o')+1])


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


command[command.index('-o')+1] = str(out/'unchanged-matrix.o')
run(*command)
assert (out/'unchanged-matrix.o').read_bytes() == original.read_bytes(), 'ENTIRE original matrix object FIRST'
matrix = out/'vector-metadata-program.o'
command[command.index('-o')+1] = str(matrix)
command[command.index('-c')+1] = str(here/'blackhole_qwen38_vector_q4_metadata_math.cpp')
command = [command[0], '-DPIONEER_Q38_VECTOR_Q4_METADATA_IMAGE=1',
           '-DPIONEER_Q38_VECTOR_Q4_METADATA=1', *command[1:]]
run(*command)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
(out/'program.ld').write_bytes((control/'program.ld').read_bytes())
link = parent['link'][:]
assert link.count(str(original)) == 1
link = [str(matrix) if p == str(original) else p for p in link]
for option, name in (('-T', 'program.ld'), ('-Map', 'program.map'), ('-o', 'program.elf')):
    link[link.index(option)+1] = str(out/name)
run(*link)
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf = out/'program.elf'
assert not subprocess.check_output([tool+'nm', '-u', str(elf)]).strip()
symbols = subprocess.check_output([tool+'nm', '-n', str(elf)], text=True)
names = subprocess.check_output([tool+'nm', '-nC', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
for name in ('PioneerQ38VectorQ4Metadata::Block<VectorPanelEmitter>', 'TwinBatchTilePrefetchStage(',
             'q38_compute_once', 'PioneerQ38WideReturningEmitter'):
    assert name in names, name
assert 'PioneerQ38CachedAffine::BlockQ4(' not in names
assert 'PioneerQ38BlockAffineDst<' not in names
dis = subprocess.check_output([tool+'objdump', '-d', str(elf)], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
reserved = [row for row in dis.splitlines() if re.search(r'\bs11\b', row)]
assert len(reserved) == 2
assert sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', row)) for row in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', row)) for row in reserved) == 1
frames = [row.split('\t') for row in matrix.with_suffix('.su').read_text().splitlines()]
assert frames and all(len(r) == 3 and r[2] == 'static' and int(r[1]) <= 4096 for r in frames)
sections = parent['sections']
rows = subprocess.check_output([tool+'objdump', '-h', str(elf)], text=True).splitlines()
for i, row in enumerate(rows):
    m = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if m and 'ALLOC' in rows[i+1]:
        assert m[1][1:] in sections, row
for section in sections:
    run(tool+'objcopy', '-O', 'binary', '-j', '.'+section, elf, out/(section+'.bin'))
for section, limit in (('fetch', 0x700), ('affine', 0x1500), ('text', 0x2000), ('service', 0x6000)):
    assert 0 < (out/(section+'.bin')).stat().st_size <= limit
for section in ('boot', 'precise', 'dense'):
    assert (out/(section+'.bin')).read_bytes() == (control/(section+'.bin')).read_bytes(), section
relocations = audit(control, out, tool)
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(dis)
run(sys.executable, here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py', out/'program.disassembly')
observed = []
for name in (*(section+'.bin' for section in sections), 'program.elf'):
    raw = (out/name).read_bytes()
    assert raw and not len(raw)%4
    item = (name, len(raw), hashlib.sha256(raw).hexdigest())
    observed.append(item)
    print(item, flush=True)
if not inspect:
    assert tuple(observed) == PINS, 'candidate is not independently frozen'
print('BUILD_ONLY complete returning vector-Q4-metadata candidate/ENTIRE old70f+matrix FIRST/'
      'original Q4 bytes/no weight approximation/NO execution,native,physical,Work,TPS or speedup grant', flush=True)
