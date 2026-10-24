#!/usr/bin/env python3
"""Exact COMPILE-ONLY50 role/inert dispatch;NO hardware/Work/reference route."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
valid = dict.fromkeys(('PIONEER_Q38_GDN_GATED_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE',
    'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE',
    'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_DECODER_RUNTIME', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
    'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_LEARNED_RMS_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE', 'TT_PIONEER_DRIVER_RELOAD'), '1')
valid['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010050ULL'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
def preprocess(flags, accepted):
    global count
    r = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E',
                        *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                       input=prefix, capture_output=True, text=True, env=clean)
    assert (r.returncode == 0) == accepted, (flags, r.stderr)
    count += 1
preprocess({}, True)
preprocess(valid, True)
for key in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[key]
        else:
            flags[key] = value
        preprocess(flags, False)
for tag in ('0x2026102200010038ULL', '0x2026102200010042ULL', '0x2026102300010043ULL',
            '0x2026102300010044ULL', '0x2026102300010045ULL', '0x2026102300010046ULL',
            '0x2026102400010047ULL', '0x2026102400010048ULL', '0x2026102400010049ULL'):
    preprocess({**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
preprocess({'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for flag in ('PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE',
             'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE', 'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',
             'PIONEER_Q38_EMBEDDING_QUALIFICATION', 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE',
             'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE', 'PIONEER_Q38_CPU_VALIDATION',
             'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        preprocess({**valid, flag: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-qgh-native-gates-') as directory:
    base = pathlib.Path(directory)
    for name in ('PioneerQ38EmbeddingValidationReference.h', 'PioneerQ38LearnedRmsValidationReference.h',
                 'PioneerQ38ProjectionValidationReference.h', 'PioneerQ38ResidentDecoderValidationReference.h',
                 'PioneerQ38GdnInputValidationReference.h', 'PioneerQ38GdnRecurrentValidationReference.h'):
        client = 'PIONEER_Q38_GDN_RECURRENT_VALIDATION_CLIENT' if name == 'PioneerQ38GdnRecurrentValidationReference.h' else 'PIONEER_Q38_GDN_INPUT_VALIDATION_CLIENT'
        for value in (None, '0', '1'):
            flags = [] if value is None else ['-DPIONEER_Q38_GDN_GATED_NATIVE=' + value]
            r = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-x', 'c++', '-E',
                                '-D' + client + '=1', '-DPIONEER_Q38_CPU_VALIDATION=1',
                                '-DPIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE=1', *flags, '-'],
                               input='#include "' + str(here.parent / name) + '"\n',capture_output=True,text=True,env=clean)
            assert (r.returncode == 0) == (value is None), (name, value, r.stderr)
            count += 1
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            r = subprocess.run(['sh', str(here.parent / ('build_qwen38_' + name + '.sh')),
                                str(base / 'MUST_NOT_READ_SDK'), str(base / 'MUST_NOT_CREATE_OUTPUT')],
                               capture_output=True, text=True, env={**clean, 'PIONEER_Q38_GDN_GATED_NATIVE': value}, timeout=5)
            assert r.returncode == 2 and not r.stdout and not r.stderr and not (base / 'MUST_NOT_CREATE_OUTPUT').exists(), (name, value, r.returncode, r.stdout, r.stderr)
            count += 1
    for key in ('PIONEER_Q38_GDN_GATED_NATIVE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION'):
        for value in ('1', '2', '', 'false'):
            r = subprocess.run([sys.executable, str(here.parent / 'build_qwen38_gdn_gated_native.py'),
                                str(base / 'MUST_NOT_READ_SDK'), str(base / 'MUST_NOT_CREATE_OUTPUT')],
                               capture_output=True, text=True, env={**clean, key: value}, timeout=5)
            assert r.returncode == 2 and not r.stdout and not r.stderr and not (base / 'MUST_NOT_CREATE_OUTPUT').exists()
            count += 1
    begin, end = '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_BEGIN', '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0]
    code = '#include <cassert>\nusing status_t=int;constexpr int B_NOT_ALLOWED=-1;int touches=0;\n'
    code += 'int Route(unsigned operation){(void)operation;\n' + route + '\n touches++;return 8;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++)assert(Route(op)==B_NOT_ALLOWED);assert(!touches);}\n'
    path, binary = base / 'routes.cpp', base / 'routes'
    path.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True, env=clean)
    subprocess.run([str(binary)], check=True, env=clean)
print(f'PASS NATIVE50 exact role/tag/mixed/old-builder gates{count};131072 ACTUAL inert dispatch probes;NO public compute/upload/Work/CPU-reference selector or hardware authority')
