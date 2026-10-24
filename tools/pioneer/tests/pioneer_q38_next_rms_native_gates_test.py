#!/usr/bin/env python3
"""Actual native58 prefix/parents/inert routes/private producer/CPU exclusion."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
valid = dict.fromkeys(('PIONEER_Q38_NEXT_RMS_NATIVE', 'PIONEER_Q38_FFN_RESIDUAL_NATIVE', 'PIONEER_Q38_RESIDUAL_RMS_NATIVE',
    'PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE', 'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE',
    'PIONEER_Q38_GDN_GATED_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_STATE_INIT_NATIVE',
    'PIONEER_Q38_GDN_INPUT_OWNER', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_OWNER',
    'PIONEER_Q38_DECODER_RUNTIME', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH',
    'PIONEER_Q38_LEARNED_RMS_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE', 'TT_PIONEER_DRIVER_RELOAD'), '1')
valid['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010058ULL'
count = 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(prefix, {}, True)
check(prefix, valid, True)
for flag in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[flag]
        else:
            flags[flag] = value
        check(prefix, flags, False)
for tag in ('0x2026102200010038ULL', '0x2026102300010043ULL', '0x2026102400010051ULL',
            '0x2026102400010052ULL', '0x2026102400010053ULL', '0x2026102400010054ULL',
            '0x2026102400010055ULL', '0x2026102400010056ULL', '0x2026102400010057ULL'):
    check(prefix, {**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
check(prefix, {'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for flag in ('PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT', 'PIONEER_Q38_RESIDUAL_RMS_IMAGE', 'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE',
             'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE',
             'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
             'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_QUALIFICATION',
             'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        check(prefix, {**valid, flag: value}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput'):
    client = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    flags = {'PIONEER_Q38_'+client+'_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, flags, True)
    for value in ('0', '1', '2', '-1'):
        check(text, {**flags, 'PIONEER_Q38_NEXT_RMS_NATIVE': value}, False)
    for value in ('0', '1'):
        check(text+'#define PIONEER_Q38_NEXT_RMS_NATIVE '+value+'\n'+text, flags, False)
with tempfile.TemporaryDirectory(prefix='pioneer-native58-gates-') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv],
                                    capture_output=True, text=True, env={**clean, 'PIONEER_Q38_NEXT_RMS_NATIVE': value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists(), (name, value, result.stderr)
            count += 1
    for builder, runner in (('build_qwen38_next_rms_native.py', sys.executable), ('build_qwen38_ffn_residual_native.py', sys.executable),
                            ('build_qwen38_residual_rms_native.py', sys.executable), ('build_qwen38_gdn_output_execution_native.py', sys.executable),
                            ('build_qwen38_gdn_output_validation_client.sh', 'sh')):
        for flag in ('PIONEER_Q38_NEXT_RMS_NATIVE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '', 'false'):
                result = subprocess.run([runner, str(directory/builder), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                        capture_output=True, text=True, env={**clean, flag: value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
    begin, end = '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_BEGIN', '// PIONEER_Q38_GDN_RECURRENT_NATIVE_INERT_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0]
    code = '#include <cassert>\n#include <type_traits>\n#include "'+str(kernel/'PioneerQ38NextRmsProducer.h')+'"\n'
    code += 'static_assert(!std::is_default_constructible<PioneerQ38NextRmsProducer>::value && !std::is_copy_constructible<PioneerQ38NextRmsProducer>::value && !std::is_move_constructible<PioneerQ38NextRmsProducer>::value);\n'
    code += 'using status_t=int;constexpr int B_NOT_ALLOWED=-1;unsigned touches=0;\nint Route(unsigned operation){(void)operation;\n'+route+'\ntouches++;return 9;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++)assert(Route(op)==B_NOT_ALLOWED);assert(!touches);}\n'
    path, binary = root/'routes.cpp', root/'routes';path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-DPIONEER_Q38_NEXT_RMS_NATIVE=1', '-DPIONEER_Q38_FFN_RESIDUAL_NATIVE=1',
                    '-DPIONEER_Q38_RESIDUAL_RMS_NATIVE=1', '-DPIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE=1', str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print(f'PASS native58 exact role/tag/parents/mixed+defined-zero/reference+reinclude/private producer/builder gates{count}/131072 ACTUAL inert compute routes;NO CPU products/Work/public selector')
