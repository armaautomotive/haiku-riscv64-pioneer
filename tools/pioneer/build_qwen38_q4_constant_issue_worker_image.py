#!/usr/bin/env python3
"""Separate full returning worker BUILD; no native/install/device IO."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent/'tests'))
from pioneer_q38_q4_constant_relocations import audit

PINS = (
    ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin',9232,'6d565f73e3f9856a252632ef278165fb78b477102c03a72f44f07320474e23bd'),
    ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin',1292,'63102a2e454bc149c11e44af086ce3ca0fc8853b06ab3fe092169753c076c6d5'),
    ('text.bin',6408,'d99ce0af068f3dfbd42df71747a2beca2c32701e959f0c5de7f467572d9ef93a'),
    ('affine.bin',5360,'472b4a921fa0df587eebce11b2706c6e5d128d67e1a3e2b6bd5a37509e7155d1'),
    ('ffn.bin',16456,'23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin',9888,'7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin',35944,'c2824d4822c69867879b09fac67f9f9849cf5f7202c1beca668584976eaf4e9c'),
    ('program.elf',144844,'e9ef5c53dbb3f8a924920c5dac261a884bbe33c3168d90cfd168e02fa96d8397'),
)  # Independent main/clean BUILD identity only, not execution authority.
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
command = (control/'unchanged-twin'/'compile-math-argv.txt').read_text().splitlines()
original = pathlib.Path(command[command.index('-o')+1])


def run(*values):
    subprocess.run(list(map(str, values)), check=True)


unchanged = out/'unchanged-matrix.o'
command[command.index('-o')+1] = str(unchanged)
run(*command)
assert unchanged.read_bytes() == original.read_bytes(), 'ENTIRE current matrix object must reproduce FIRST'
matrix = out/'constant-q4-program.o'
command[command.index('-o')+1] = str(matrix)
command[command.index('-c')+1] = str(here/'blackhole_qwen38_q4_constant_issue_math.cpp')
command = [command[0], '-DPIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE=1',
           '-DPIONEER_Q38_Q4_CONSTANT_ISSUE=1', *command[1:]]
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
for name in ('PioneerQ38Q4ConstantTransform<VectorPanelEmitter, 0u>',
             'PioneerQ38Q4ConstantTransform<VectorPanelEmitter, 2u>',
             'TwinBatchTilePrefetchStage(', 'q38_compute_once', 'PioneerQ38WideReturningEmitter'):
    assert name in names, name
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
for section, limit in (('fetch', 0x700), ('affine', 0x1500), ('text', 0x2000)):
    assert 0 < (out/(section+'.bin')).stat().st_size <= limit
for section in ('boot', 'precise', 'dense', 'ffn', 'service', 'gdn'):
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
    assert tuple(observed) == PINS, 'new full constant Q4 worker is not independently frozen'
print('BUILD_ONLY constant Q4 instruction issue/ENTIRE current70f worker+matrix object FIRST/'
      'original replay+integer operations+drains+buffer lifetime/no new opcode or layout;'
      'NO execution,native,physical,quality,Work,TPS or speedup grant', flush=True)
