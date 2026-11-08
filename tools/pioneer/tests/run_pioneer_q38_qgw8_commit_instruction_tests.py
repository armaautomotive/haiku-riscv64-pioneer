#!/usr/bin/env python3
"""Execute the actual pinned QGW8 RV32 commit bodies against synthetic NoC."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent
old, new = map(Path, sys.argv[1:])
pins = ((old, '8b7bac64e64d963e729e347a209c3ca4be951992aa924ead0c48d97e0e79478f'),
        (new, '9ee84a211ba8a0acb13aff818711f679dd1b88ba4be9a7f385a4c0f60377d401'))
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
# QGW8 uses the BF16/hybrid layout, not the older pushed-completion layout.
# Its FFN metadata offsets differ even though CommitState geometry is shared.
flags.extend(('-DPIONEER_Q38_GDN_MUTABLE_STATE=1', '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'))
source = (here.parent / 'blackhole_qwen38_token_root.cpp').read_text()
begin = source.index('struct Private {')
private = source[begin:source.index('static volatile uint32_t* Words(', begin)]
assert 'Position=16,Token=17,Committed=18,Slot=19' in private
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-commit-rv32-', dir='/private/tmp') as name:
    temporary = Path(name)
    (temporary / 'actual_token_commit_private.inc').write_text(private)
    binary = temporary / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-I' + str(temporary),
                    str(here / 'pioneer_q38_commit_boundary_instruction_test.cpp'), '-o', str(binary)], check=True)
    for directory, digest in pins:
        assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
        elf = (directory / 'program.elf').read_bytes()
        assert hashlib.sha256(elf).hexdigest() == digest
        snapshot = temporary / 'program.elf'
        snapshot.write_bytes(elf)
        for section in ('text', 'conv', 'terminal', 'bf16'):
            path = temporary / (section + '.bin')
            subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(snapshot), str(path)], check=True)
            assert path.read_bytes() == (directory / (section + '.bin')).read_bytes()
        table = subprocess.check_output([tool + 'nm', '-nC', str(snapshot)], text=True)
        entries = []
        for function in ('CommitState', 'InputSeal', 'FeedbackSeal'):
            matches = re.findall(r'^([0-9a-f]+) [tTW] GlobalChain::' + function + r'\(', table, re.M)
            assert len(matches) == 1, (function, matches)
            entries.append(matches[0])
        subprocess.run([str(binary), str(temporary), *entries, str(int(directory == new)),
                        '--synthetic-predecessor-and-noc', str(temporary / ('new' if directory == new else 'old')),
                        '--qgw8-bf16'], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    for feedback in (0, 1):
        before = (temporary / ('old-feedback' + str(feedback) + '.bin')).read_bytes()
        after = (temporary / ('new-feedback' + str(feedback) + '.bin')).read_bytes()
        assert before and after == before, 'exact command/DRAM/private/numeric/scratch/descriptor equality'
        print('PASS QGW8 feedback' + str(feedback) + ' exact old/new modeled transaction bytes=' + str(len(after)), flush=True)
print('PASS pinned QGW8 RV32 commit bodies with synthetic predecessor/NoC; NO device, model quality or TPS')
