#!/usr/bin/env python3
"""Authenticate the one-instruction admission repair;NOT connected execution."""
import ast
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 4 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root, rejected, worker = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
for directory in (root, rejected, worker):
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
tree = ast.parse((here.parent / 'build_qwen38_batched_prefetch_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
old_pins = tuple((name, size, 'c87570de8dc8ba0001db08bd6890cb7017dfbf01bd98ae08af98a873f21cdc0d'
                  if name == 'text.bin' else 'b622da7fdfa314c9ee4d7695f066d374de86c47d98c5ed79e4e88d0fc972cca8'
                  if name == 'program.elf' else digest) for name, size, digest in pins)
snapshots = []
for directory, records in ((root, pins), (rejected, old_pins)):
    source = {}
    for name, size, digest in records:
        data = (directory / name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)
        source[name] = data
    snapshots.append(source)
new, old = snapshots
for section in ('boot.bin', 'terminal.bin', 'conv.bin'):
    assert new[section] == old[section]
words = [struct.unpack('<' + str(len(v['text.bin']) // 4) + 'I', v['text.bin']) for v in snapshots]
changed = [i for i, pair in enumerate(zip(*words)) if pair[0] != pair[1]]
assert len(changed) == 1
at = changed[0]
assert words[0][at - 3:at + 3] == (0x002a1793, 0xf1042703, 0x014787b3, 0x00379313, 0x00679793, 0x00230313)
assert words[1][at] == 0x00579313
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
table = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(root / 'program.elf')], text=True)
stages = [row for row in map(str.split, table.splitlines())
          if len(row) == 4 and row[3] == '_ZN20MixedRecurrentCohortL9StageConvEPj']
assert len(stages) == 1
start, size = (int(value, 16) for value in stages[0][:2])
assert start <= 0x40000 + at * 4 < start + size  # actual inlined admission body,not a DATA word


def execute(sequence, groups):
    regs = [0] * 32
    regs[20] = groups
    for instruction in sequence:
        opcode, rd, funct3, rs1, rs2 = (instruction & 127, instruction >> 7 & 31,
                                       instruction >> 12 & 7, instruction >> 15 & 31, instruction >> 20 & 31)
        if opcode == 0x13 and funct3 == 1:
            value = regs[rs1] << rs2
        elif opcode == 0x13 and funct3 == 0:
            value = regs[rs1] + (instruction >> 20)
        else:
            assert opcode == 0x33 and funct3 == 0 and instruction >> 25 == 0
            value = regs[rs1] + regs[rs2]
        regs[rd] = value & 0xffffffff
    return regs[6], regs[15]


for groups in range(1, 17):
    # Only this local arithmetic calculation is interpreted here. The
    # interleaved actual LW writes a4,not a5/t1/s4,and is not interpreted
    # with a fake memory endpoint. The full native test still executes it.
    new_arithmetic = (words[0][at - 3], *words[0][at - 1:at + 3])
    old_arithmetic = (words[1][at - 3], *words[1][at - 1:at + 3])
    assert execute(new_arithmetic, groups) == (groups * 40 + 2, groups * 320)
    assert execute(old_arithmetic, groups) == (groups * 160 + 2, groups * 320)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-rejected-batched-root-', dir='/private/tmp') as temporary:
    output = pathlib.Path(temporary) / 'MUST_NOT_EXIST.h'
    result = subprocess.run([sys.executable, str(here.parent / 'generate_qwen38_token_programs.py'),
                             str(rejected), str(worker), str(output), '--batched-prefetch'], capture_output=True)
    assert result.returncode == 2 and b'text.bin' in result.stderr and not output.exists()
print(f'PASS batched cohort ROOT revision/ALL5 authenticated artifacts/ONLY1 text instruction changed at{0x40000 + at * 4:x}'
      '/actual StageConv inlined admission/16 old+new count calculations/unrepaired whole predecessor refused before output;'
      'NOT connected native lifecycle,model arithmetic,hardware or TPS proof')
