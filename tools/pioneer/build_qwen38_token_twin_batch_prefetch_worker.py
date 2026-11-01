#!/usr/bin/env python3
"""Separate copy-free twin-batch shader BUILD;NO DATA/native/hardware grant."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

PINS = (
    ('boot.bin',4,'a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c'),
    ('globalworker.bin',9232,'b583b806f676b4a1bdfcf988e900d71b63336a950cab5dc3a51e18c7171ee94c'),
    ('precise.bin',8392,'32c078029e1c74ec4bd9744ea494433f5003f63a936df05445d3066c722a7ecc'),
    ('dense.bin',10332,'d1749707ffe803e28dced9c7d065e763f125009678ace33b4d468109984869ad'),
    ('fetch.bin',1292,'8422f5a12befcf4bba1500919006a79c00accbe29ffa99c1750dff66dc871f77'),
    ('text.bin',6004,'d5fc264b4cb0a76ae84a68bd5762ff620b1b3eea38fd9bb3a34cbef4e76b4d8d'),
    ('affine.bin',5360,'0074182efb372d30e38807d26f701901c3995a5d3a549110b4ac35e5800ae51f'),
    ('ffn.bin',16456,'23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin',9888,'7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin',35408,'cbe3faf60ef0b3a0544b27b29f99d62eb36c7d0ac5b835e337c5501661ed589c'),
    ('program.elf',143880,'56b470ae508655a070f17294c0a0fe242335214faed7101abaa05fb385ce6e38'),
)  # Independent main/clean BUILD identity only,NOT execution authority.
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 2 and (inspect or PINS)
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
sdk, out = map(pathlib.Path, args)
here = pathlib.Path(__file__).resolve().parent
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'build_qwen38_token_batched_prefetch_worker.py'), str(sdk), str(out/'unchanged-batched')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
control = out/'unchanged-batched'
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest() == 'b73a8278ed2bf6f7c9e8713072b01ecf2a6e28206003026f4d1fbd5ff3c41c8e'
tool = parent['tool']
def run(*values):
    subprocess.run(list(map(str, values)), check=True)

matrix = out/'twin-batch-program.o'
command = parent['command'][:]
command[command.index('-o')+1] = str(matrix)
command[command.index('-c')+1] = str(here/'blackhole_qwen38_twin_batch_prefetch_math.cpp')
command = [command[0], '-DPIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE=1',
           '-DPIONEER_Q38_WORK_TWIN_BATCH_PREFETCH=1', *command[1:]]
run(*command)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')
# No sender or ROOT count change:identical original Q8x4/Q4x2/Q6x2 packets.
# Changing the raw-buffer lifetime still REQUIRES a distinct future DATA
# package,actual execution/lifecycle qualification and immutable identities.
(out/'program.ld').write_bytes((control/'program.ld').read_bytes())
link = parent['link'][:]
assert link.count(str(parent['matrix'])) == 1
link = [str(matrix) if value == str(parent['matrix']) else value for value in link]
link[link.index('-T')+1] = str(out/'program.ld')
link[link.index('-Map')+1] = str(out/'program.map')
link[link.index('-o')+1] = str(out/'program.elf')
run(*link)
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
elf = out/'program.elf'
assert not subprocess.check_output([tool+'nm', '-u', str(elf)], text=True).strip()
symbols = subprocess.check_output([tool+'nm', '-n', str(elf)], text=True)
names = subprocess.check_output([tool+'nm', '-nC', str(elf)], text=True)
assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', symbols, re.M), 'no writable globals/GOT'
for name in ('TwinBatchTilePrefetchStage(', 'TilePrefetchStart(', 'TilePrefetchWait(',
             'PioneerQ38TwinBatchPrefetchPlan::RawInput(', 'q38_compute_once', 'q38_global_worker_main'):
    assert name in names, name
assert not re.search(r'\b(?:BatchedTilePrefetchStage|TilePrefetchStage)\(', names)
assert 'PioneerQ38BatchedPrefetchPlan::RawInput(' not in names
disassembly = subprocess.check_output([tool+'objdump', '-d', str(elf)], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', disassembly)
reserved = [line for line in disassembly.splitlines() if re.search(r'\bs11\b', line)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', line)) for line in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', line)) for line in reserved) == 1
frames = [int(line.split('\t')[1]) for line in matrix.with_suffix('.su').read_text().splitlines()]
assert frames and max(frames) <= 4096, max(frames)
sections = ('boot', 'globalworker', 'precise', 'dense', 'fetch', 'text', 'affine', 'ffn', 'service', 'gdn')
rows = subprocess.check_output([tool+'objdump', '-h', str(elf)], text=True).splitlines()
for index, row in enumerate(rows):
    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
    if match and 'ALLOC' in rows[index+1]:
        assert match[1][1:] in sections, row
for section in sections:
    run(tool+'objcopy', '-O', 'binary', '-j', '.'+section, elf, out/(section+'.bin'))
for section, limit in (('fetch', 0x700), ('affine', 0x1500), ('text', 0x2000)):
    assert 0 < (out/(section+'.bin')).stat().st_size <= limit, (section, (out/(section+'.bin')).stat().st_size)
for section in ('boot', 'globalworker', 'precise', 'dense', 'ffn', 'service', 'gdn'):
    assert (out/(section+'.bin')).read_bytes() == (control/(section+'.bin')).read_bytes(), section
(out/'program.symbols').write_text(symbols)
(out/'program.disassembly').write_text(disassembly)
run(sys.executable, here/'tests/pioneer_q38_gdn_rendezvous_csr_gate.py', out/'program.disassembly')
observed = []
for name in (*(section+'.bin' for section in sections), 'program.elf'):
    data = (out/name).read_bytes()
    assert data and not len(data)%4
    item = (name, len(data), hashlib.sha256(data).hexdigest())
    observed.append(item)
    print(item, flush=True)
if not inspect:
    assert tuple(observed) == PINS, 'new twin-batch subject is not independently frozen'
print('BUILD_ONLY twin-batch prefetch/ENTIRE oldb73+ancestors FIRST/unchanged sender+packet counts/'
      'two complete disjoint batches98000+9a000/SAME saved L1/no raw copies in new source/'
      'actual previous-batch drain required;NO emitted execution,native,physical,Work,TPS or speedup grant', flush=True)
