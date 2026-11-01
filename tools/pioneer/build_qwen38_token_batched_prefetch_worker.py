#!/usr/bin/env python3
"""Distinct batched-prefetch worker BUILD only;old ROOT/native are INCOMPATIBLE."""
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
    ('fetch.bin',1260,'d8ed95db94cdc9b969f450892f2023e9aab1b1f11993c593cbdbb62abc0c5310'),
    ('text.bin',6004,'cf54005dd8501ccaaa08108455f32deaf6a32876dc606781563a65813c03e4b3'),
    ('affine.bin',5360,'0074182efb372d30e38807d26f701901c3995a5d3a549110b4ac35e5800ae51f'),
    ('ffn.bin',16456,'23ce6cc2dc07aa445b7babcac12a068dc3542001329cd24831e8e6749ff3a16f'),
    ('service.bin',9888,'7d5a42a087bc6bf9d24319583f3655ae344828a51b1092154f7b0567b0fdb187'),
    ('gdn.bin',35408,'cbe3faf60ef0b3a0544b27b29f99d62eb36c7d0ac5b835e337c5501661ed589c'),
    ('program.elf',143888,'b73a8278ed2bf6f7c9e8713072b01ecf2a6e28206003026f4d1fbd5ff3c41c8e'),
)  # Independent main/clean BUILD identity ONLY,not execution authority.
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
    sys.argv = [str(here/'build_qwen38_token_prefetch_worker.py'), str(sdk), str(out/'unchanged-prefetch')]
    parent = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
control = out/'unchanged-prefetch'
assert hashlib.sha256((control/'program.elf').read_bytes()).hexdigest() == 'cf02c9777314443510894903a74637277be1ccc9b0db8ae80e7fd900baea7120'
tool = parent['tool']
def run(*values):
    subprocess.run(list(map(str, values)), check=True)

matrix = out/'batched-program.o'
command = parent['new_command'][:]
command[command.index('-o')+1] = str(matrix)
command[command.index('-c')+1] = str(here/'blackhole_qwen38_batched_prefetch_math.cpp')
command = [command[0], '-DPIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE=1',
           '-DPIONEER_Q38_WORK_BATCHED_PREFETCH=1', *command[1:]]
run(*command)
(out/'compile-math-argv.txt').write_text('\n'.join(command)+'\n')

# Rebuild the ENTIRE old sender object FIRST from its original command,
# before changing its read-count grammar. New math must NEVER be linked
# under the old one-read-per-tile counter expectations.
original_global = parent['control']/'unchanged-pushed/global.o'
global_command = [tool+'g++', *parent['ancestor']['flags'], '-Os', '-ffixed-s11',
                  '-DPIONEER_Q38_PRECISE_WORKER_IMAGE=1', '-DPIONEER_Q38_PUSH_COMPLETION_WORKER_IMAGE=1',
                  '-c', str(here/'blackhole_qwen38_global_worker.cpp'), '-o', str(out/'unchanged-global.o')]
run(*global_command)
assert (out/'unchanged-global.o').read_bytes() == original_global.read_bytes(), 'ENTIRE old global counter/receipt object must reproduce'
sender = out/'batched-global.o'
global_command[-1] = str(sender)
global_command[global_command.index('-c')+1] = str(here/'blackhole_qwen38_batched_prefetch_global.cpp')
global_command = [global_command[0], '-DPIONEER_Q38_TOKEN_BATCHED_PREFETCH_WORKER_IMAGE=1', *global_command[1:]]
run(*global_command)
(out/'compile-global-argv.txt').write_text('\n'.join(global_command)+'\n')
script = (control/'program.ld').read_text()
anchor = '*(.text.pioneer_q38_batch4_stage)'
assert script.count(anchor) == 1
# Separate COMDAT input section avoids a section-type conflict with the
# local stage functions. BOTH remain inside the SAME bounded fetch output.
script = script.replace(anchor, anchor+' *(.text.pioneer_q38_batched_raw_input)')
(out/'program.ld').write_text(script)
link = parent['link'][:]
replacements = {str(parent['obj']):str(matrix), str(original_global):str(sender)}
assert all(link.count(old) == 1 for old in replacements)
link = [replacements.get(value, value) for value in link]
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
for name in ('BatchedTilePrefetchStage(', 'TilePrefetchStart(', 'TilePrefetchWait(',
             'PioneerQ38BatchedPrefetchPlan::RawInput(', 'q38_compute_once', 'q38_global_worker_main'):
    assert name in names, name
assert not re.search(r'\bTilePrefetchStage\(', names), 'old one-tile stage retained'
disassembly = subprocess.check_output([tool+'objdump', '-d', str(elf)], text=True)
assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|ebreak)\b', disassembly)
reserved = [line for line in disassembly.splitlines() if re.search(r'\bs11\b', line)]
assert len(reserved) == 2 and sum(bool(re.search(r'\bmv\s+s11,\w+\s*$', line)) for line in reserved) == 1
assert sum(bool(re.search(r'\bmv\s+\w+,s11\s*$', line)) for line in reserved) == 1
for obj, bound in ((matrix, 4096), (sender, 2048)):
    frames = [int(line.split('\t')[1]) for line in obj.with_suffix('.su').read_text().splitlines()]
    assert frames and max(frames) <= bound, (obj, max(frames))
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
for section in ('boot', 'precise', 'dense', 'ffn', 'service', 'gdn'):
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
    assert tuple(observed) == PINS, 'new batched subject is not independently frozen'
print('BUILD_ONLY batched-prefetch math AND sender counter grammar/ENTIRE oldcf02+ancestors+global object FIRST/'
      'same original quantized bytes/SAME saved L1/<=8KiB precaution/real drain before scratch overwrite;'
      'OLD ROOT/NATIVE COUNTERS INCOMPATIBLE;NO execution,native,hardware,Work,TPS or speedup grant', flush=True)
