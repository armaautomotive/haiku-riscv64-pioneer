#!/usr/bin/env python3
"""Actual native factories/scope/END with emitted instructions;NO hardware."""
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
mode = sys.argv[7] if len(sys.argv) >= 8 else ''
gdn = mode in ('--validation-gdn-input', '--validation-gdn-input-extremes')
if (len(sys.argv) != 9 if gdn else len(sys.argv) not in (7, 8)) or mode not in ('', '--faults', '--history-faults', '--validation', '--validation-faults', '--validation-control', '--validation-control-faults', '--validation-close-only', '--validation-capture', '--validation-actual-model', '--validation-actual-model-extremes', '--validation-actual-model-frontend-pc', '--validation-gdn-input', '--validation-gdn-input-extremes'):
    raise SystemExit('expected QPS1 QFP3 ZERO QPE1 QRN1 QCB1 [--faults|--history-faults|--validation[-faults|-control[-faults]]]')
validation = mode.startswith('--validation')
actual = gdn or mode in ('--validation-actual-model', '--validation-actual-model-extremes', '--validation-actual-model-frontend-pc')
control = mode in ('--validation-control', '--validation-control-faults', '--validation-close-only') or actual
if gdn:
    assert hashlib.sha256((pathlib.Path(sys.argv[8]) / 'code.bin').read_bytes()).hexdigest() == 'bcc15ddc0e76c0a432a63502201f9c2451366b02615e3cddc9cf64bb147757c1'
    assert (pathlib.Path(sys.argv[8]) / 'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
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
flags += ['-DPIONEER_Q38_PROJECTION_RESULT_STASH=1', '-DPIONEER_Q38_DECODER_RUNTIME=1',
          '-DPIONEER_Q38_RESIDENT_DECODER_OWNER=1', '-DPIONEER_Q38_RESIDENT_DECODER_NATIVE=1']
if validation:
    flags.append('-DPIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE=1')
with tempfile.TemporaryDirectory(prefix='pioneer-decoder-native-factory-') as temporary:
    base = pathlib.Path(temporary)
    for prefix, family, names in [('embedding', 'EMBEDDING', ('scope', 'unsafe', 'mapping', 'factory', 'end')),
                                  ('rms', 'LEARNED_RMS', ('scope', 'factory')),
                                  ('projection', 'PROJECTION_EXECUTION', ('scope', 'unsafe', 'factory', 'end')),
                                  ('decoder', 'RESIDENT_DECODER', ('scope', 'factory'))]:
        for name in names:
            marker = 'PIONEER_Q38_' + family + '_NATIVE_' + name.upper()
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert source.count(begin) == source.count(end) == 1
            (base / (prefix + '_factory_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    for target, marker in [('embedding_factory_retain.inc', 'PIONEER_Q38_RETAIN_MEMORY'),
                           ('projection_factory_mapping.inc', 'PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER')]:
        begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
        assert source.count(begin) == source.count(end) == 1
        (base / target).write_text(source.split(begin, 1)[1].split(end, 1)[0])
    begin, end = 'static bool\nEndQ38DispatchOwnership(', '// PIONEER_Q38_DISPATCH_RETAIN_END'
    assert source.count(begin) == source.count(end) == 1
    (base / 'projection_factory_complete_end.inc').write_text(begin + source.split(begin, 1)[1].split(end, 1)[0])
    if control:
        for target, marker in [('decoder_validation_control.inc', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_CONTROL'),
                               ('decoder_validation_routing.inc', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_ROUTING'),
                               ('decoder_validation_probe_lock.inc', 'PIONEER_PROBE_LOCK'),
                               ('decoder_validation_call_scope.inc', 'PIONEER_Q38_GDN_CALL_SCOPE'),
                               ('decoder_validation_cookie_release.inc', 'PIONEER_Q38_DISPATCH_COOKIE_RELEASE')]:
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert source.count(begin) == source.count(end) == 1
            (base / target).write_text(source.split(begin, 1)[1].split(end, 1)[0])
        begin = 'static status_t\nProbeControl(void* cookie, uint32 operation, void* data, size_t length)\n{'
        assert source.count(begin) == 1
        prelude = begin + source.split(begin, 1)[1].split('#ifdef TT_PIONEER_DRIVER_RELOAD', 1)[0]
        assert 'geteuid() != 0' in prelude and 'instance->magic!=0x50313030' in prelude
        (base / 'decoder_validation_route_prelude.inc').write_text(prelude)
    binary = base / 'test'
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined']
    objects = []
    if actual:
        observer = base / 'original-byte-observer.o'
        subprocess.run([*common, '-c', str(here / 'pioneer_q38_decoder_original_source.cpp'), '-o', str(observer)], check=True)
        objects.append(str(observer)) # NO native flags;original references never compile into native controllers
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, '-DPIONEER_Q38_GDN_MUTABLE_STATE=', '-DPIONEER_Q38_NORMAL_CLOCK=1',
                    '-DPIONEER_Q38_EMBEDDING_NATIVE=1', '-DPIONEER_Q38_LEARNED_RMS_NATIVE=1', '-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1',
                    '-I' + str(base), *[str(here / (name + '.cpp')) for name in (
                        'pioneer_q38_gdn_input_original_model_test' if gdn else 'pioneer_q38_resident_decoder_validation_actual_model_test' if actual else 'pioneer_q38_resident_decoder_validation_close_test' if mode == '--validation-close-only' else 'pioneer_q38_resident_decoder_validation_control_test' if control else
                        'pioneer_q38_resident_decoder_validation_capture_test' if mode == '--validation-capture' else 'pioneer_q38_resident_decoder_validation_native_test' if validation else 'pioneer_q38_resident_decoder_native_factory_test', 'pioneer_q38_projection_native_transport',
                        'pioneer_q38_projection_execution_machine', 'pioneer_q38_embedding_execution_bridge',
                        'pioneer_q38_learned_rms_execution_bridge', 'pioneer_q38_decoder_conv_bridge')],
                    *([str(here / 'pioneer_q38_gdn_input_bridge.cpp')] if gdn else []), *objects, '-o', str(binary)], check=True)
    cases = list(range(9)) if mode == '--validation-control-faults' else [None]
    for case in cases:
        suffix = [str(case)] if case is not None else ['--faults'] if mode == '--validation-faults' else [] if validation else sys.argv[7:]
        if actual:
            suffix = [str(here.parents[2] / 'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf')]
            if gdn:
                suffix.append(sys.argv[8])
                if mode == '--validation-gdn-input-extremes':
                    suffix.append('--all-tokens')
            elif mode == '--validation-actual-model-extremes':
                suffix.append('--all-tokens')
            elif mode == '--validation-actual-model-frontend-pc':
                suffix.append('--frontend-pc')
        subprocess.run([str(binary), *sys.argv[1:7], *suffix], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
