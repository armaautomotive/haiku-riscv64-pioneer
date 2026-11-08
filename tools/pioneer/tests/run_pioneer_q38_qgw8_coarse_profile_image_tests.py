#!/usr/bin/env python3
"""Authenticate and inspect linked QGW8 coarse timing helpers; no card IO."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 4
assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
sdk, baseline, profile = map(Path, sys.argv[1:])
assert all(path.is_absolute() and path.is_dir() for path in (sdk, baseline, profile))
here = Path(__file__).resolve().parent
tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
pins = {
    'boot.bin': '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca',
    'text.bin': '210d46bc28e8922db955047ff8731881928623f5dd868541467a25710fa73167',
    'terminal.bin': '6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5',
    'conv.bin': '73e03eb67845cb3d6e0c06296524d3d71f64d2f838fa31246feb5d2dfd06934a',
    'bf16.bin': '981deb140842b92d34510e215bd4df285002a173e1935b66e7475935fc6b5800',
    'program.elf': '7be485638726e4ee50bbc3af62e9aeed3b73b0a739ab68029fd4d9b397cc095c',
}
assert hashlib.sha256((baseline / 'program.elf').read_bytes()).hexdigest() == '8b7bac64e64d963e729e347a209c3ca4be951992aa924ead0c48d97e0e79478f'
for name, pin in pins.items():
    assert hashlib.sha256((profile / name).read_bytes()).hexdigest() == pin, name
for name in ('token-components-NOBOOT.o', 'unchanged-feedback/feedback.o'):
    assert (profile / 'qualified-components' / name).read_bytes() == (baseline / 'qualified-components' / name).read_bytes(), name

elf = profile / 'program.elf'
table = subprocess.check_output([tool + 'objdump', '-t', str(elf)], text=True)
intervals = {}
for row in map(str.split, table.splitlines()):
    if len(row) == 6 and row[2] == 'F' and int(row[4], 16):
        first, size = int(row[0], 16), int(row[4], 16)
        intervals.setdefault((first, first + size), set()).add(row[5])
disassembly = subprocess.check_output([tool + 'objdump', '-d', str(elf)], text=True)
assert disassembly == (profile / 'program.disassembly').read_text()
calls = {}
for match in re.finditer(r'^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s.*$', disassembly, re.M):
    call = re.search(r'\bjalr\b.*# [0-9a-f]+ <(q38_token_phase_[a-z]+)>', match[0])
    if not call:
        continue
    owner = [names for (first, last), names in intervals.items() if first <= int(match[1], 16) < last]
    assert len(owner) == 1 and len(owner[0]) == 1
    key = (next(iter(owner[0])), call[1])
    calls[key] = calls.get(key, 0) + 1
assert calls == {
    ('q38_global_chain_main', 'q38_token_phase_begin'): 1,
    ('q38_global_chain_main', 'q38_token_phase_mark'): 3,
    ('_ZN11GlobalChainL6FinishEPKjj', 'q38_token_phase_mark'): 1,
    ('_ZN11GlobalChainL6FinishEPKjj', 'q38_token_phase_complete'): 1,
    ('_ZN11GlobalChainL7RunHeadEv', 'q38_token_phase_mark'): 9,
    ('q38_token_phase_begin', 'q38_token_phase_clock'): 1,
    ('q38_token_phase_mark', 'q38_token_phase_clock'): 1,
    ('q38_token_phase_complete', 'q38_token_phase_clock'): 1,
}, calls

with tempfile.TemporaryDirectory(prefix='qgw8-coarse-image-', dir='/private/tmp') as name:
    temporary = Path(name)
    for section in ('boot', 'text', 'terminal', 'conv', 'bf16'):
        out = temporary / (section + '.bin')
        subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(elf), str(out)], check=True)
        assert out.read_bytes() == (profile / (section + '.bin')).read_bytes()
    test = temporary / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here / 'pioneer_q38_token_phase_profile_image_test.cpp'),
                    '-o', str(test)], check=True)
    subprocess.run([str(test), str(profile)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    output = temporary / 'MUST_NOT_EXIST.h'
    result = subprocess.run([sys.executable, str(here.parent / 'generate_qwen38_hybrid_programs.py'),
        '--qgw8-native-candidate', str(profile), str(temporary / 'MUST_NOT_OPEN_WORKER'), str(output)], capture_output=True, text=True)
    assert result.returncode and 'artifact text.bin' in result.stderr and 'FileNotFoundError' not in result.stderr and not output.exists()
print('PASS pinned QGW8 baseline/profile sections, unchanged layer/feedback objects, emitted timing calls, helper execution and old-family refusal; NO DEVICE/WORK/SPEED')
