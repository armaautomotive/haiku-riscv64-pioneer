#!/usr/bin/env python3
"""Execute authenticated linked RV32; SYNTHETIC unit predecessor and NoC only."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
old, new = map(pathlib.Path, sys.argv[1:])
pins = ((old, '48b2bf31d2eefc36e50dcaaa9202d7d92579496a5ab72e9a62c9d4cfb66250ee'),
        (new, '83677f78f548b8fc11d874a858b670310d1dad859982e737c674b2ff7329db00'))
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
flags.append('-DPIONEER_Q38_GDN_MUTABLE_STATE=1')
# Canonical dense DATA must use the SAME pushed-receipt SRAM geometry as
# both authenticated ROOTs; this does not replace any executed guard body.
flags.extend(('-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
              '-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1'))
source = (here.parent / 'blackhole_qwen38_token_root.cpp').read_text()
begin = source.index('struct Private {')
private = source[begin:source.index('static volatile uint32_t* Words(', begin)]
assert 'Position=16,Token=17,Committed=18,Slot=19' in private
with tempfile.TemporaryDirectory(prefix='pioneer-q38-commit-rv32-', dir='/private/tmp') as name:
    temporary = pathlib.Path(name)
    (temporary / 'actual_token_commit_private.inc').write_text(private)
    binary = temporary / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-I' + str(temporary),
                    str(here / 'pioneer_q38_commit_boundary_instruction_test.cpp'), '-o', str(binary)], check=True)
    for directory, digest in pins:
        assert directory.is_absolute() and not directory.is_symlink()
        # Run ONLY private copies of the exact authenticated ELF sections.
        elf = (directory / 'program.elf').read_bytes()
        assert hashlib.sha256(elf).hexdigest() == digest
        snapshot = temporary / 'program.elf'
        snapshot.write_bytes(elf)
        for section in ('text', 'conv', 'terminal'):
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
                        '--synthetic-predecessor-and-noc', str(temporary / ('new' if directory == new else 'old'))], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    for feedback in (0, 1):
        before = (temporary / ('old-feedback' + str(feedback) + '.bin')).read_bytes()
        after = (temporary / ('new-feedback' + str(feedback) + '.bin')).read_bytes()
        assert before and after == before, 'exact command/DRAM/private/numeric/scratch/descriptor equality'
        print('PASS feedback' + str(feedback) + ' EXACT old/new commands+all modeled DRAM+private+numeric+'
              'head+session+scratch+descriptor bytes=' + str(len(after)), flush=True)
print('PASS authenticated old48b2/new83677 ACTUAL RV32 transaction/seal/NoC-command bodies; '
      'synthetic unit predecessor+NoC,NOT genuine model/native/physical/Work/performance proof')
