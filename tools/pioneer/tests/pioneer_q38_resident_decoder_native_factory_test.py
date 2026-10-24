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
gdn_state = mode in ('--gdn-state-init', '--gdn-state-init-faults')
gdn_gated_local = mode == '--gdn-gated-norm-local'
gdn_gated_resident = mode == '--gdn-gated-resident'
gdn_gated = gdn_gated_local or gdn_gated_resident
gdn_rendezvous_debug = mode in ('--gdn-rendezvous-debug', '--gdn-rendezvous-debug-faults')
gdn_output_debug = mode in ('--gdn-output-debug', '--gdn-output-debug-faults')
residual_rms = mode in ('--residual-rms', '--residual-rms-faults')
gdn_output_execution = residual_rms or gdn_output_debug or mode in ('--gdn-output-execution', '--gdn-output-execution-faults')
gdn_output_producer = gdn_output_execution or mode == '--gdn-output-producer'
gdn_rendezvous = gdn_output_producer or gdn_rendezvous_debug or mode in ('--gdn-rendezvous-native', '--gdn-rendezvous-native-delayed', '--gdn-rendezvous-native-host-deadline', '--gdn-rendezvous-native-clock-receipt-fault', '--gdn-rendezvous-native-controller-faults')
gdn_gated_native = gdn_rendezvous or mode in ('--gdn-gated-native', '--gdn-gated-native-late-negative')
gdn_recurrent_debug = mode in ('--gdn-recurrent-debug', '--gdn-recurrent-debug-faults')
gdn_recurrent = gdn_gated_native or gdn_gated or gdn_recurrent_debug or mode in ('--gdn-recurrent', '--gdn-recurrent-faults')
gdn_debug = mode in ('--gdn-debug', '--gdn-debug-faults')
gdn_native = gdn_debug or mode in ('--gdn-native', '--gdn-native-faults')
gdn = gdn_native or gdn_recurrent or mode in ('--validation-gdn-input', '--validation-gdn-input-extremes')
if (len(sys.argv) != (12 if residual_rms else 11 if gdn_output_execution else 10 if gdn_gated else 9) if gdn else len(sys.argv) not in (7, 8)) or mode not in ('--residual-rms', '--residual-rms-faults', '--gdn-output-debug', '--gdn-output-debug-faults', '--gdn-output-execution-faults', '--gdn-output-execution', '--gdn-output-producer', '--gdn-rendezvous-debug', '--gdn-rendezvous-debug-faults', '', '--faults', '--history-faults', '--validation', '--validation-faults', '--validation-control', '--validation-control-faults', '--validation-close-only', '--validation-capture', '--validation-actual-model', '--validation-actual-model-extremes', '--validation-actual-model-frontend-pc', '--validation-gdn-input', '--validation-gdn-input-extremes', '--gdn-native', '--gdn-native-faults', '--gdn-debug', '--gdn-debug-faults', '--gdn-state-init', '--gdn-state-init-faults', '--gdn-recurrent', '--gdn-recurrent-faults', '--gdn-recurrent-debug', '--gdn-recurrent-debug-faults', '--gdn-gated-norm-local', '--gdn-gated-resident', '--gdn-gated-native', '--gdn-gated-native-late-negative', '--gdn-rendezvous-native', '--gdn-rendezvous-native-delayed', '--gdn-rendezvous-native-host-deadline', '--gdn-rendezvous-native-clock-receipt-fault', '--gdn-rendezvous-native-controller-faults'):
    raise SystemExit('expected QPS1 QFP3 ZERO QPE1 QRN1 QCB1 [--faults|--history-faults|--validation[-faults|-control[-faults]]]')
validation = mode.startswith('--validation')
actual = gdn or mode in ('--validation-actual-model', '--validation-actual-model-extremes', '--validation-actual-model-frontend-pc')
control = mode in ('--validation-control', '--validation-control-faults', '--validation-close-only') or actual
if gdn:
    assert hashlib.sha256((pathlib.Path(sys.argv[8]) / 'code.bin').read_bytes()).hexdigest() == ('3a99ac89d98236b7e3dac5e1cf433223961fdb1b7f0f6f80dd81d541a47b0284' if gdn_rendezvous else '22e3001215d67c9b9f8861098db5e5fda76fb241076c0ece8adb3ba224dfe297' if gdn_gated_native else '96881b6501e4febea5ed869c1e30e26a8bb432ce45001b7cd1fe93234e50a410' if gdn_recurrent else 'bcc15ddc0e76c0a432a63502201f9c2451366b02615e3cddc9cf64bb147757c1')
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
if gdn_native:
    flags.append('-DPIONEER_Q38_GDN_INPUT_OWNER=1')
if gdn_debug:
    flags.append('-DPIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE=1')
if gdn_state or gdn_recurrent:
    flags += ['-DPIONEER_Q38_GDN_INPUT_OWNER=1', '-DPIONEER_Q38_GDN_STATE_INIT_NATIVE=1']
if gdn_recurrent:
    flags.append('-DPIONEER_Q38_GDN_RECURRENT_NATIVE=1')
if gdn_gated_native:
    flags.append('-DPIONEER_Q38_GDN_GATED_NATIVE=1')
if gdn_rendezvous_debug:
    flags.append('-DPIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE=1')
if gdn_output_debug:
    flags.append('-DPIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE=1')
if residual_rms:
    flags.append('-DPIONEER_Q38_RESIDUAL_RMS_NATIVE=1')
    assert hashlib.sha256((pathlib.Path(sys.argv[11])/'code.bin').read_bytes()).hexdigest() == '93976e38dfc9f310a7415ce83bc6cabc2ebf5c15ac10c1745fee79d1f2c61989'
    assert (pathlib.Path(sys.argv[11])/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
if gdn_rendezvous:
    flags.append('-DPIONEER_Q38_GDN_RENDEZVOUS_NATIVE=1')
if gdn_output_producer:
    flags.append('-DPIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE=1')
if gdn_output_execution:
    flags.append('-DPIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE=1')
    output_pins=runpy.run_path(str(here.parent/'generate_qwen38_gdn_output_images.py'))['PINS']
    for name,(section,size,pin,_) in output_pins.items():
        image=pathlib.Path(sys.argv[9 if name.startswith('root-') else 10])
        data=(image/(section+'.bin')).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==pin
        assert (image/'boot.bin').read_bytes()==struct.pack('<I',0x0006c06f)
if gdn_gated_local:
    flags.append('-DPIONEER_Q38_TEST_GDN_GATED_NORM_LOCAL=1')
    assert hashlib.sha256((pathlib.Path(sys.argv[9]) / 'code.bin').read_bytes()).hexdigest() == '7ec76ef20a0a453ae24adbda74623b5f95a400ff0f5b15e17e3788f079beb2b8'
    assert (pathlib.Path(sys.argv[9]) / 'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
if gdn_gated_resident:
    flags.append('-DPIONEER_Q38_TEST_GDN_GATED_RESIDENT=1')
    assert hashlib.sha256((pathlib.Path(sys.argv[9]) / 'code.bin').read_bytes()).hexdigest() == '22e3001215d67c9b9f8861098db5e5fda76fb241076c0ece8adb3ba224dfe297'
    assert (pathlib.Path(sys.argv[9]) / 'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
if gdn_recurrent_debug:
    flags.append('-DPIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE=1')
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
    if gdn_state or gdn_recurrent:
        gate = ''
        for signature in ('bool PioneerSessionIO::StateZeroBeforeCohort()\n{', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()\n{'):
            assert source.count(signature) == 1
            gate += signature + source.split(signature, 1)[1].split('\n}', 1)[0] + '\n}\n'
        (base / 'gdn_state_zero_gate.inc').write_text(gate)
    if gdn_recurrent:
        begin, end = '// PIONEER_Q38_GDN_RECURRENT_NATIVE_SCOPE_BEGIN', '// PIONEER_Q38_GDN_RECURRENT_NATIVE_SCOPE_END'
        assert source.count(begin) == source.count(end) == 1
        (base / 'gdn_recurrent_scope.inc').write_text(source.split(begin, 1)[1].split(end, 1)[0])
    if control:
        for target, marker in [('decoder_validation_control.inc', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_CONTROL'),
                               ('decoder_validation_routing.inc', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE_ROUTING'),
                               ('decoder_validation_probe_lock.inc', 'PIONEER_PROBE_LOCK'),
                               ('decoder_validation_call_scope.inc', 'PIONEER_Q38_GDN_CALL_SCOPE'),
                               ('decoder_validation_cookie_release.inc', 'PIONEER_Q38_DISPATCH_COOKIE_RELEASE')]:
            begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
            assert source.count(begin) == source.count(end) == 1
            (base / target).write_text(source.split(begin, 1)[1].split(end, 1)[0])
        if gdn_debug:
            for target, marker in [('gdn_input_validation_control.inc', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE_CONTROL'),
                                  ('gdn_input_validation_routing.inc', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE_ROUTING')]:
                begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
                assert source.count(begin) == source.count(end) == 1
                (base / target).write_text(source.split(begin, 1)[1].split(end, 1)[0])
        if gdn_recurrent_debug:
            for name in ('control', 'routing'):
                marker = 'PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE_' + name.upper()
                begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
                assert source.count(begin) == source.count(end) == 1
                (base / ('gdn_recurrent_validation_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
        if gdn_rendezvous_debug:
            for name in ('control', 'routing'):
                marker = 'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE_' + name.upper()
                begin, end = '// ' + marker + '_BEGIN', '// ' + marker + '_END'
                assert source.count(begin) == source.count(end) == 1
                (base / ('gdn_rendezvous_validation_' + name + '.inc')).write_text(source.split(begin, 1)[1].split(end, 1)[0])
        if gdn_output_debug:
            begin, end = '// PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE_ROUTING_BEGIN', '// PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE_ROUTING_END'
            assert source.count(begin) == source.count(end) == 1
            (base/'gdn_output_validation_routing.inc').write_text(source.split(begin, 1)[1].split(end, 1)[0])
            assert source.count('#include "PioneerQ38GdnOutputValidationControl.h"') == 1
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
                        'pioneer_q38_gdn_output_validation_control_test' if gdn_output_debug else 'pioneer_q38_gdn_rendezvous_validation_control_test' if gdn_rendezvous_debug else 'pioneer_q38_gdn_gated_native_factory_test' if gdn_gated_native else 'pioneer_q38_gdn_recurrent_validation_control_test' if gdn_recurrent_debug else 'pioneer_q38_gdn_recurrent_native_factory_test' if gdn_recurrent else 'pioneer_q38_gdn_state_init_native_factory_test' if gdn_state else 'pioneer_q38_gdn_input_validation_control_test' if gdn_debug else 'pioneer_q38_gdn_input_native_factory_test' if gdn_native else 'pioneer_q38_gdn_input_original_model_test' if gdn else 'pioneer_q38_resident_decoder_validation_actual_model_test' if actual else 'pioneer_q38_resident_decoder_validation_close_test' if mode == '--validation-close-only' else 'pioneer_q38_resident_decoder_validation_control_test' if control else
                        'pioneer_q38_resident_decoder_validation_capture_test' if mode == '--validation-capture' else 'pioneer_q38_resident_decoder_validation_native_test' if validation else 'pioneer_q38_resident_decoder_native_factory_test', 'pioneer_q38_projection_native_transport',
                        'pioneer_q38_projection_execution_machine', 'pioneer_q38_embedding_execution_bridge',
                        'pioneer_q38_learned_rms_execution_bridge', 'pioneer_q38_decoder_conv_bridge')],
                    *([str(here / 'pioneer_q38_gdn_input_bridge.cpp')] if gdn or gdn_state else []),
                    *([str(here / 'pioneer_q38_gdn_recurrent_bridge.cpp')] if gdn_recurrent else []),
                    *([str(here / 'pioneer_q38_gdn_gated_norm_bridge.cpp')] if gdn_gated_local else []),
                    *([str(here / 'pioneer_q38_gdn_gated_resident_bridge.cpp')] if gdn_gated_resident else []),
                    *([str(here / 'pioneer_q38_gdn_gated_native_bridge.cpp')] if gdn_gated_native else []),
                    *([str(here / 'pioneer_q38_gdn_output_execution_machine.cpp')] if gdn_output_execution else []),
                    *([str(here / 'pioneer_q38_residual_rms_execution_bridge.cpp')] if residual_rms else []), *objects, '-o', str(binary)], check=True)
    cases = list(range(9)) if mode == '--validation-control-faults' else [None]
    for case in cases:
        suffix = [str(case)] if case is not None else ['--faults'] if mode == '--validation-faults' else [] if validation else sys.argv[7:]
        if actual:
            suffix = [str(here.parents[2] / 'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf')]
            if gdn:
                suffix.append(sys.argv[8])
                if gdn_output_execution:
                    suffix.extend(sys.argv[9:11])
                    if residual_rms:
                        suffix.append(sys.argv[11])
                        if mode == '--residual-rms-faults':
                            suffix.append('--residual-faults')
                    if mode=='--gdn-output-execution-faults':
                        suffix.append('--output-faults')
                    if mode=='--gdn-output-debug-faults':
                        suffix.append('--faults')
                if gdn_gated:
                    suffix.append(sys.argv[9])
                if mode == '--validation-gdn-input-extremes':
                    suffix.append('--all-tokens')
                elif mode in ('--gdn-native-faults', '--gdn-debug-faults', '--gdn-recurrent-faults', '--gdn-recurrent-debug-faults', '--gdn-rendezvous-debug-faults'):
                    suffix.append('--faults')
                elif mode == '--gdn-gated-native-late-negative':
                    suffix.append('--late-negative')
                elif mode == '--gdn-rendezvous-native-delayed':
                    suffix.append('--delayed')
                elif mode == '--gdn-rendezvous-native-controller-faults':
                    suffix.append('--controller-faults')
                elif mode == '--gdn-rendezvous-native-clock-receipt-fault':
                    suffix.append('--clock-receipt-fault')
                elif mode == '--gdn-rendezvous-native-host-deadline':
                    suffix.append('--host-deadline')
            elif mode == '--validation-actual-model-extremes':
                suffix.append('--all-tokens')
            elif mode == '--validation-actual-model-frontend-pc':
                suffix.append('--frontend-pc')
        subprocess.run([str(binary), *sys.argv[1:7], *suffix], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
