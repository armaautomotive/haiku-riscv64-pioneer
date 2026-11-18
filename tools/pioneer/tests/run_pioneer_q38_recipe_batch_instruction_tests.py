#!/usr/bin/env python3
"""Run the two pinned RV32 senders with synthetic NoC, without a card."""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

PINS = (
    '7f2f2d4895c72c3278ce83676be5b9a122e0a074d319b76b86586611630627af',
    'ee6e0765744b935bc299e597e1b10859aa944e9114b9bbed915e135cb299be29',
)
FIXTURES = '5a95c1c42983751153df493c2540d9b6501520d8'


def main():
    if len(sys.argv) != 4:
        raise SystemExit('use SDK PINNED_V4_ROOT PINNED_RECIPE_BATCH_ROOT')
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    sdk, *images = map(Path, sys.argv[1:])
    assert all(p.is_absolute() for p in (sdk, *images))
    here = Path(__file__).resolve().parent
    tool = str(sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-recipe-rv32-') as scratch:
        tmp = Path(scratch)
        inputs = []
        for i, (source, digest) in enumerate(zip(images, PINS)):
            data = (source / 'program.elf').read_bytes()
            assert hashlib.sha256(data).hexdigest() == digest, 'wrong complete ELF input'
            current = tmp / str(i)
            current.mkdir()
            elf = current / 'root.elf'
            elf.write_bytes(data)
            # No caller-supplied symbol file or section is trusted.
            for section in ('text', 'conv', 'terminal'):
                raw = current / (section + '.bin')
                subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, raw], check=True)
                assert raw.read_bytes() == (source / raw.name).read_bytes()
            symbols = subprocess.check_output([tool + 'nm', '-nC', elf], text=True)
            matches = re.findall(r'^([0-9a-f]+) t GlobalChain::TokenRecipes\(', symbols, re.M)
            assert len(matches) == 1
            assert re.search(r'^0*60000 T scalar_fault_park$', symbols, re.M)
            assert re.search(r'^0*60004 T q38_global_chain_stopped$', symbols, re.M)
            inputs.extend((str(current), matches[0]))
        # Recreate v4 DATA from the same frozen vector92 source as the
        # images. The current tree's GDN cohort has different geometry.
        tree = tmp / 'frozen'
        tree.mkdir()
        with subprocess.Popen(['git', '-C', here.parents[2], 'archive', FIXTURES, 'tools/pioneer',
                               'src/add-ons/kernel/drivers/misc'], stdout=subprocess.PIPE) as archive:
            with archive.stdout:
                subprocess.run(['tar', '-x', '-C', tree], stdin=archive.stdout, check=True)
            assert archive.wait() == 0
        name = 'pioneer_q38_recipe_batch_instruction_test.cpp'
        unit = tree / 'tools/pioneer/tests' / name
        assert not unit.exists()
        shutil.copyfile(here / name, unit)
        binary = tmp / 'test'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
                        '-ffp-contract=off', '-fsanitize=address,undefined', '-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
                        '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1',
                        '-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
                        unit, '-o', binary], check=True)
        subprocess.run([binary, *inputs], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32',
                                                           'UBSAN_OPTIONS': 'halt_on_error=1'})


if __name__ == '__main__':
    main()
