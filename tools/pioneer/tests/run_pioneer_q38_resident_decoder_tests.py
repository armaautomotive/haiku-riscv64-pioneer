#!/usr/bin/env python3
"""Actual retained arithmetic controllers/instructions,OFFLINE endpoints only."""
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
if len(sys.argv) not in (7, 8) or (len(sys.argv) == 8 and sys.argv[7] != '--faults'):
    raise SystemExit('expected QPS1 QFP3 ZERO QPE1 QRN1 QCB1 directories [--faults]')
pins = runpy.run_path(str(here.parent / 'generate_qwen38_projection_stash_images.py'))['PINS']
for name, (section, size, sha, _) in pins.items():
    directory = pathlib.Path(sys.argv[1 if name.startswith('root-') else 2])
    data = (directory / (section + '.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
    assert (directory / 'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
for directory, name, sha in zip(sys.argv[3:7], ('text.bin', 'code.bin', 'code.bin', 'code.bin'), (
        '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b',
        'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1',
        'f13d8676f97920c182fa47d9f555453d7855dd5ac394919a577d58c83abe523a',
        'a8df2da24fa98535c70172c6f9833228c996a540f487b66fa545660b93256350')):
    assert hashlib.sha256((pathlib.Path(directory) / name).read_bytes()).hexdigest() == sha
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
flags = sorted(set(re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here / 'run_pioneer_q38_projection_graph_cooperative_tests.sh').read_text())))
flags.remove('-DPIONEER_Q38_TEST_PROJECTION_FOOTPRINT')
with tempfile.TemporaryDirectory(prefix='pioneer-resident-decoder-') as temporary:
    base = pathlib.Path(temporary)
    for prefix, family, names in [('embedding', 'EMBEDDING', ('scope', 'unsafe', 'mapping', 'factory', 'end')),
                                   ('rms', 'LEARNED_RMS', ('scope', 'factory'))]:
        for name in names:
            marker = 'PIONEER_Q38_' + family + '_NATIVE_' + name.upper()
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert source.count(begin) == source.count(end) == 1
            (base / (prefix + '_factory_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    begin, end = '// PIONEER_Q38_RETAIN_MEMORY_BEGIN', '// PIONEER_Q38_RETAIN_MEMORY_END'
    assert source.count(begin) == source.count(end) == 1
    (base / 'embedding_factory_retain.inc').write_text(source.split(begin, 1)[1].split(end, 1)[0])
    binary = base / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-DPIONEER_Q38_NORMAL_CLOCK=1',
                    '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1',
                    '-DPIONEER_Q38_PROJECTION_RESULT_STASH=1', '-DPIONEER_Q38_DECODER_RUNTIME=1', '-DPIONEER_Q38_RESIDENT_DECODER_OWNER=1',
                    '-I' + str(base), *[str(here / (name + '.cpp')) for name in (
                        'pioneer_q38_resident_decoder_owner_test', 'pioneer_q38_projection_execution_producer',
                        'pioneer_q38_projection_execution_machine', 'pioneer_q38_embedding_execution_bridge',
                        'pioneer_q38_learned_rms_execution_bridge', 'pioneer_q38_decoder_conv_bridge')], '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
