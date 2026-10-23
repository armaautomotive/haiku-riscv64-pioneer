#!/usr/bin/env python3
"""Actual same-context native factories+installed instructions,offline only."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
stashes = sys.argv[-1:] == ['--result-stashes']
if stashes:
    sys.argv.pop()
if len(sys.argv) not in (6, 7) or (len(sys.argv) == 7 and sys.argv[6] not in ('--wide', '--faults', '--actual-model', '--actual-model-extremes')):
    raise SystemExit('expected QPR1 QFP3 ZERO QPE1 QRN1 [--wide|--faults|--actual-model|--actual-model-extremes]')
actual = sys.argv[-1] in ('--actual-model', '--actual-model-extremes')
pins = runpy.run_path(str(here.parent / ('generate_qwen38_projection_stash_images.py' if stashes else 'generate_qwen38_projection_graph_images.py')))['PINS']
for name, (section, size, sha, _) in pins.items():
    directory = pathlib.Path(sys.argv[1 if name.startswith('root-') else 2])
    data = (directory / (section + '.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
    assert (directory / 'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for directory, name, sha in zip(sys.argv[3:6], ('text.bin', 'code.bin', 'code.bin'), (
        '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b',
        'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1',
        'f13d8676f97920c182fa47d9f555453d7855dd5ac394919a577d58c83abe523a')):
    assert hashlib.sha256((pathlib.Path(directory) / name).read_bytes()).hexdigest() == sha
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
flags = sorted(set(re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_projection_graph_cooperative_tests.sh').read_text())))
flags.remove('-DPIONEER_Q38_TEST_PROJECTION_FOOTPRINT')
if stashes:
    flags.append('-DPIONEER_Q38_PROJECTION_RESULT_STASH=1')
with tempfile.TemporaryDirectory(prefix='pioneer-projection-native-factory-') as temporary:
    base = pathlib.Path(temporary)
    for prefix, family, names in [
            ('embedding', 'EMBEDDING', ('scope', 'unsafe', 'mapping', 'factory', 'end')),
            ('rms', 'LEARNED_RMS', ('scope', 'factory')),
            ('projection', 'PROJECTION_EXECUTION', ('scope', 'unsafe', 'factory', 'end'))]:
        for name in names:
            marker = 'PIONEER_Q38_' + family + '_NATIVE_' + name.upper()
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert source.count(begin) == source.count(end) == 1
            body = source.split(begin, 1)[1].split(end, 1)[0]
            if prefix == 'projection' and name == 'unsafe':
                # Extract the real dangling else WITH its actual follower.
                follower = 'if (q38Dispatch && PioneerQ38DispatchUnsafe(*q38Dispatch)) return true;'
                assert source.split(end, 1)[1].lstrip().startswith('#endif\n\t\t' + follower)
            (base / (prefix + '_factory_' + name + '.inc')).write_text(body)
    for target, marker in [('embedding_factory_retain.inc', 'PIONEER_Q38_RETAIN_MEMORY'),
                           ('projection_factory_mapping.inc', 'PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER')]:
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (base / target).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    begin, end = 'static bool\nEndQ38DispatchOwnership(', '// PIONEER_Q38_DISPATCH_RETAIN_END'
    assert source.count(begin) == source.count(end) == 1
    (base / 'projection_factory_complete_end.inc').write_text(begin + source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-DPIONEER_Q38_NORMAL_CLOCK=1',
                    '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1',
                    '-I' + str(base), *[str(here / (name + '.cpp')) for name in (
                        'pioneer_q38_projection_actual_model_test' if actual else 'pioneer_q38_projection_execution_native_factory_test', 'pioneer_q38_projection_native_transport',
                        'pioneer_q38_projection_execution_machine', 'pioneer_q38_embedding_execution_bridge',
                        'pioneer_q38_learned_rms_execution_bridge')], '-o', str(binary)], check=True)
    cases = list(range(14)) + list(range(20, 26)) if sys.argv[-1] == '--faults' else [None]
    for case in cases:
        suffix = ['--wide'] if sys.argv[-1] == '--wide' else [] if case is None else [str(case)]
        if actual:
            suffix = [str(here.parents[2] / 'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf')]
            if sys.argv[-1] == '--actual-model-extremes':
                suffix += ['--all-tokens']
        subprocess.run([str(binary), *sys.argv[1:6], *suffix], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
