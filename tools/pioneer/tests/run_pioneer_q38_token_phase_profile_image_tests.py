#!/usr/bin/env python3
"""Pinned whole ROOT / actual timing helpers, no owner or accelerator IO."""
import ast
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root = pathlib.Path(sys.argv[1])
here = pathlib.Path(__file__).resolve().parent
assert root.is_absolute()
tree = ast.parse((here.parent / 'build_qwen38_token_phase_profile_root.py').read_text())
pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
assert pins, 'entire independently reproduced profile BUILD must be frozen first'
snapshots = {}
for name, size, digest in pins:
    data = (root / name).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    snapshots[name] = data
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
symbols = subprocess.check_output([tool + 'nm', '-n', str(root / 'program.elf')], text=True)
assert symbols == (root / 'program.symbols').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-q38-token-phase-image-', dir='/private/tmp') as name:
    temporary = pathlib.Path(name)
    for section in ('boot', 'text', 'terminal', 'conv'):
        output = temporary / (section + '.bin')
        subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, str(root / 'program.elf'), str(output)], check=True)
        assert output.read_bytes() == snapshots[section + '.bin']
    binary = temporary / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here / 'pioneer_q38_token_phase_profile_image_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    # No existing native program family may silently accept the new ROOT.
    generator = here.parent / 'generate_qwen38_token_programs.py'
    options = ('--cancel-boundary', '--no-cancel', '--precise-activation', '--pushed-completion', '--fault-marker',
               '--precise-beta', '--job-boot-fold', '--token-frontier', '--retained-conv-closure', '--commit-boundary-seals',
               '--cohort-local-sources', '--packet-identity-boundary', '--tid-boundary', '--counter-gather',
               '--empty-receipt-poll', '--flat-matrix-grammar', '--tile-prefetch', '--batched-prefetch',
               '--twin-batch-prefetch', '--batched-output-closure', '--wide-gdn')
    for number, option in enumerate(options):
        output = temporary / ('REFUSED-' + str(number) + '.h')
        result = subprocess.run([sys.executable, str(generator), str(root), str(temporary / 'MUST_NOT_OPEN_WORKER'),
                                 str(output), option], capture_output=True, text=True)
        assert result.returncode and result.stderr.startswith('PROGRAM_DATA_REFUSAL artifact ') and not output.exists(), result.stderr
        assert 'FileNotFoundError' not in result.stderr, 'must refuse full ROOT identity before worker IO'
print('PASS entire pinned ROOT authenticated/linked-section bytes exact/actual timing-helper instructions/'
      'ALL21 existing program-native families refuse before worker IO or output;'
      'NOT full ROOT,original-model,native,physical clock,Work or TPS qualification')
