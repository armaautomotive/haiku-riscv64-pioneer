#!/usr/bin/env python3
"""Execute emitted gather/guard with queued synthetic4B endpoints, not a model."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 1
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
candidate = pathlib.Path(args[0])
assert candidate.is_absolute() and not candidate.is_symlink()
here = pathlib.Path(__file__).resolve().parent
tree = ast.parse((here.parent / 'build_qwen38_counter_gather_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert inspect or pins
for name, size, digest in pins:
    data = (candidate / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
assert '-DPIONEER_Q38_COUNTER_GATHER_IMAGE=1' in (candidate / 'compile-argv.txt').read_text().splitlines()
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
symbols = subprocess.check_output([tool + 'nm', '-nC', str(candidate / 'program.elf')], text=True)
names = [(int(row[0], 16), row[2]) for line in symbols.splitlines()
         if len(row := line.split(maxsplit=2)) == 3 and row[2].startswith('PioneerQ38CounterGather::Read(')]
assert len(names) == 1, names
entry = names[0][0]
seal_names = [(int(row[0], 16), row[2]) for line in symbols.splitlines()
              if len(row := line.split(maxsplit=2)) == 3 and row[2].startswith('MixedRecurrentCohort::CounterSeal(')]
assert len(seal_names) == 1, seal_names
seal_entry = seal_names[0][0]
fault_names = [int(row[0], 16) for line in symbols.splitlines()
               if len(row := line.split()) == 3 and row[2] == 'q38_token_fault_report']
assert len(fault_names) == 1
fault_park = fault_names[0] + 28
assert int.from_bytes((candidate / 'text.bin').read_bytes()[fault_park-0x40000:fault_park-0x40000+4], 'little') == 0x0000006f
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',
                   (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-counter-gather-instructions-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp) / 'test'
    mapping = pathlib.Path(tmp) / 'entries'
    mapping.write_text(' '.join(format(value, 'x') for value in (entry, seal_entry, fault_park)) + '\n')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags,
                    str(here / 'pioneer_q38_counter_gather_instruction_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(candidate), str(mapping)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
