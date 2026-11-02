#!/usr/bin/env python3
"""Authenticate actual ROOT count instruction; not connected admission/model IO."""
import ast
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root, old = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
snapshots = []
for directory, script in ((root, 'build_qwen38_batched_output_closure_root.py'),
                          (old, 'build_qwen38_batched_prefetch_root.py')):
    assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    tree = ast.parse((here.parent / script).read_text())
    pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
    assert pins
    source = {}
    for name, size, digest in pins:
        path = directory / name
        assert path.is_file() and not path.is_symlink()
        data = path.read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)
        source[name] = data
    snapshots.append(source)
new, previous = snapshots
for section in ('boot.bin', 'terminal.bin', 'conv.bin'):
    assert new[section] == previous[section]
assert len(new['text.bin']) == len(previous['text.bin'])
words = [struct.unpack('<' + str(len(v['text.bin']) // 4) + 'I', v['text.bin']) for v in snapshots]
changed = [i for i, pair in enumerate(zip(*words)) if pair[0] != pair[1]]
assert len(changed) == 1
at = changed[0]
pc = 0x40000 + at * 4
assert words[0][at] == 0x00a00613 and words[1][at] == 0x01400613  # li a2,10 vs li a2,20
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
tables = []
for directory in (root, old):
    table = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(directory / 'program.elf')], text=True)
    assert subprocess.check_output([tool + 'nm', '-n', str(directory / 'program.elf')], text=True) == \
        (directory / 'program.symbols').read_text()
    functions = [row for row in map(str.split, table.splitlines())
                 if len(row) == 4 and row[3] == '_ZN18DensePartitionRootL17AdmitGlobalOutputEPKjPjj']
    assert len(functions) == 1
    start, size = (int(value, 16) for value in functions[0][:2])
    assert start <= pc < pc + 4 <= start + size
    tables.append(table)
assert tables[0] == tables[1], 'No symbol address/extent may drift unnoticed'
for directory, snapshot in zip((root, old), snapshots):
    sections = subprocess.check_output([tool + 'objdump', '-h', str(directory / 'program.elf')], text=True)
    text = [row for row in map(str.split, sections.splitlines()) if len(row) == 7 and row[1] == '.text']
    assert len(text) == 1
    size, address, offset = int(text[0][2], 16), int(text[0][3], 16), int(text[0][5], 16)
    assert address == 0x40000 and size == len(snapshot['text.bin']) and offset == 0x2000
    assert snapshot['program.elf'][offset:offset + size] == snapshot['text.bin']
where = 0x2000 + at * 4
assert new['program.elf'] == previous['program.elf'][:where] + struct.pack('<I', words[0][at]) + \
    previous['program.elf'][where + 4:], 'The ENTIRE ELF may differ ONLY at this instruction'
# Recompilation of ALL three TUs is checked by the builder. Only recurrent
# emits this closure; the other two new objects must stay ENTIRELY exact.
for kind in ('dense', 'token'):
    assert (root / ('output-closure-' + kind + '.o')).read_bytes() == (root / ('unchanged-' + kind + '.o')).read_bytes()


def arithmetic(sequence, groups):
    # Interpret ONLY the emitted straight-line arithmetic between the new
    # LI and its counter dispatch. No fake memory input or receipt is read.
    regs = [0] * 32
    regs[20], regs[9] = groups, 0x12345678
    for instruction in sequence:
        opcode, rd, funct3, rs1, rs2 = (instruction & 127, instruction >> 7 & 31,
                                       instruction >> 12 & 7, instruction >> 15 & 31, instruction >> 20 & 31)
        if opcode == 0x37:
            value = instruction & 0xfffff000
        elif opcode == 0x13 and funct3 == 0:
            immediate = instruction >> 20
            value = regs[rs1] + (immediate if immediate < 2048 else immediate - 4096)
        else:
            assert opcode == 0x33 and funct3 == 0 and instruction >> 25 == 1
            value = regs[rs1] * regs[rs2]
        if rd:
            regs[rd] = value & 0xffffffff
    return regs[12], regs[15], regs[10], regs[30], regs[14]


for groups in range(1, 17):
    # Last exclusive instruction is the actual BLTU dispatch at pc+48.
    assert (words[0][at + 12] & 0x707f) == 0x6063
    assert arithmetic(words[0][at:at + 12], groups) == (groups * 10 + 2, groups * 1060 + 162, 0, 8, 0x12345678)
    assert arithmetic(words[1][at:at + 12], groups) == (groups * 20 + 2, groups * 1060 + 162, 0, 8, 0x12345678)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-output-closure-old-alias-', dir='/private/tmp') as temporary:
    output = pathlib.Path(temporary) / 'MUST_NOT_EXIST.h'
    # Old generator must reject the new ROOT before reading any worker or
    # creating output. No new bytes silently inherit old native authority.
    result = subprocess.run([sys.executable, str(here.parent / 'generate_qwen38_token_programs.py'),
                             str(root), str(pathlib.Path(temporary) / 'NO_WORKER'), str(output),
                             '--twin-batch-prefetch'], capture_output=True)
    assert result.returncode == 2 and b'text.bin' in result.stderr and not output.exists()
print(f'PASS batched output closure ROOT/ALL5 old+new whole artifacts/ONLY1 text instruction at{pc:x}/'
      'actual AdmitGlobalOutput symbol interval/ENTIRE ELF differs ONLY at that word/same symbol table+boot+terminal+conv/whole dense+token TUs exact/'
      '16 old+new emitted count+flit calculations/old generator refuses new ROOT BEFORE worker IO or output;'
      'NOT complete admission,worker receipts,token,native,physical,Work or TPS proof')
