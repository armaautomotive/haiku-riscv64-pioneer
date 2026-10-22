#!/usr/bin/env python3
"""Exact frozen QRN1 and ACTUAL native-role guard refusal checks,NO device."""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc'
if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen QRN1 directory')
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-rms-native-gates-') as temporary:
    base = pathlib.Path(temporary)
    generator = [sys.executable, str(here.parent / 'generate_qwen38_learned_rms_image.py')]
    result = subprocess.run([*generator, sys.argv[1]], capture_output=True, text=True, check=True)
    expected = '\n'.join(line[1:] for line in result.stdout.splitlines() if line.startswith('+')) + '\n'
    assert expected == (driver / 'PioneerQ38LearnedRmsCode.h').read_text()
    image = base / 'image'
    image.mkdir()
    for name in ('code.bin', 'boot.bin'):
        shutil.copyfile(pathlib.Path(sys.argv[1]) / name, image / name)
    for name in ('code.bin', 'boot.bin'):
        original = (image / name).read_bytes()
        for at in (0, len(original) // 2, len(original) - 1):
            changed = bytearray(original)
            changed[at] ^= 1
            (image / name).write_bytes(changed)
            result = subprocess.run([*generator, str(image)], capture_output=True, text=True)
            assert result.returncode != 0 and not result.stdout
            count += 1
        (image / name).write_bytes(original)
    # Evaluate the ACTUAL driver guard itself,not a second reimplementation.
    # No kernel includes/SDK reads or driver linking are needed for this gate.
    source = (driver / 'tenstorrent_pci_probe.cpp').read_text()
    start = '#ifdef PIONEER_Q38_LEARNED_RMS_NATIVE\n'
    assert source.count(start) >= 1
    guard = start + source.split(start, 1)[1].split('#endif\n#endif\n', 1)[0] + '#endif\n#endif\n'
    clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
    for flags in (('-DPIONEER_Q38_LEARNED_RMS_NATIVE=0',), ('-DPIONEER_Q38_LEARNED_RMS_NATIVE=2',),
                  ('-DPIONEER_Q38_LEARNED_RMS_NATIVE=1',),
                  ('-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_NATIVE=0'),
                  ('-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_NATIVE=2'),
                  ('-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_QUALIFICATION=1'),
                  ('-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_EMBEDDING_VALIDATION_NATIVE=1')):
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', *flags, '-'], input=guard,
                                env=clean, capture_output=True, text=True)
        assert result.returncode != 0 and 'Private learned RMS adapter ONLY' in result.stderr
        count += 1
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1',
                             '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-'], input=guard, env=clean, capture_output=True, text=True)
    assert result.returncode == 0
print(f'PASS private native learned RMS gates{count};exact immutable code+boot/header and actual role refusals;NO selector/link/install')
