#!/usr/bin/env python3
"""Distinct experimental QGW3 sender;BUILD ONLY,never install or run hardware."""
import ast
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

inspect = '--inspect-candidate' in sys.argv
if inspect:
    assert sys.argv.count('--inspect-candidate') == 1
    sys.argv.remove('--inspect-candidate')
assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, out = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_qwen38_precise_worker_image.py'), str(sdk), str(out/'unchanged-precise')]
    old = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
cross, flags = old['cross'], old['flags']
tree = ast.parse((here/'tests/run_pioneer_q38_precise_worker_tests.py').read_text())
control_pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                    and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
for name, size, digest in control_pins:
    data = (out/'unchanged-precise'/name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, ('unchanged QGW2', name)


def run(*args):
    subprocess.run(list(map(str, args)), check=True)


run(str(cross)+'-g++', *flags, '-Os', '-ffixed-s11', '-DPIONEER_Q38_PRECISE_WORKER_IMAGE=1',
    '-DPIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE=1', '-c', here/'blackhole_qwen38_global_worker.cpp', '-o', out/'global.o')
(out/'program.ld').write_text(old['link'])
run(str(cross)+'-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', out/'program.ld',
    out/'unchanged-precise/start.o', out/'global.o', out/'unchanged-precise/precise.o',
    *[out/'unchanged-precise/unchanged-global/qualified-math'/(name+'.o') for name in old['parent']['units']],
    '-Map', out/'program.map', '-o', out/'program.elf')
sections = old['sections']
for section in sections:
    run(str(cross)+'-objcopy', '-O', 'binary', '-j', '.'+section, out/'program.elf', out/(section+'.bin'))
symbols = subprocess.check_output([str(cross)+'-nm', '-n', str(out/'program.elf')], text=True)
assert not subprocess.check_output([str(cross)+'-nm', '-u', str(out/'program.elf')], text=True).strip()
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M)
assert re.search(r' T q38_global_worker_main$', symbols, re.M) and re.search(r' T q38_mixed_seal$', symbols, re.M)
dis = subprocess.check_output([str(cross)+'-objdump', '-d', str(out/'program.elf')], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', dis)
reserved = [line for line in dis.splitlines() if re.search(r'\bs11\b', line)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', line)) for line in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', line)) for line in reserved) == 1
frames = [int(line.split('\t')[1]) for line in (out/'global.su').read_text().splitlines()]
assert frames and max(frames) <= 2048, max(frames)
rows = subprocess.check_output([str(cross)+'-objdump', '-h', str(out/'program.elf')], text=True).splitlines()
for i, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and 'ALLOC' in rows[i+1]:
        assert match[1][1:] in sections, row
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(dis)
run(sys.executable, here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py', out/'program.disassembly')
observed = []
for name in (*(s+'.bin' for s in sections), 'program.elf'):
    data = (out/name).read_bytes()
    assert data and not len(data) % 4
    row = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(row)
    print(row)
PINS = (
    ('boot.bin', 4, 'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin', 9128, '818e2c1cf3b99d0ee438c988bd28c96dc97bcaf63887b17cbf79912dbe7b7598'),
    ('precise.bin', 8392, '32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin', 10332, 'd1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('text.bin', 5984, 'e1bd0788b82ff4f3ad58e1d527199ba4bc23ecd55936f7940d3f669c617aa419'),
    ('affine.bin', 5360, '47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78'),
    ('ffn.bin', 16456, '23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin', 9888, '7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin', 35564, '741e1a7eda2a8d6a7f01c587b2ead2bbf41402ce0cbf96cb7a9616f36c293001'),
    ('program.elf', 139684, 'ca6e8131fc9bde4a1276903bfc0d2595b3f51a8569d320ee732007de96313451'),
)  # matched independent BUILD artifacts ONLY;not runtime/native/Work/physical/TPS.
if not inspect:
    assert PINS and tuple(observed) == PINS, 'QGW3 BUILD is unqualified;inspect/match/freeze first'
print('BUILD ONLY DISTINCT QGW3/actual matrix return publishes256B payload then ACK-gated4B commit/'+
      'root existing proof slots need phase ownership+journaling/cache validation;maxIndividualFrame'+str(max(frames))+
      ';NO ROOT consumer,model/native/physical/Work/TPS qualification. DO NOT INSTALL')
