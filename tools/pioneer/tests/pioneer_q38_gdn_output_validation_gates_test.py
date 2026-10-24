#!/usr/bin/env python3
"""Exact DEBUG55 role/namespace/ABI and actual closed public allowlist."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
valid = dict.fromkeys(('PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE',
    'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE',
    'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
    'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_DECODER_RUNTIME',
    'PIONEER_Q38_PROJECTION_EXECUTION_OWNER', 'PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_LEARNED_RMS_NATIVE',
    'PIONEER_Q38_EMBEDDING_NATIVE', 'TT_PIONEER_DRIVER_RELOAD'), '1')
valid['PIONEER_DRIVER_BUILD_TAG'] = '0x2026102400010055ULL'
count = 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(prefix, {}, True)
check(prefix, valid, True)
for name in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[name]
        else:
            flags[name] = value
        check(prefix, flags, False)
for tag in ('0x2026102200010038ULL', '0x2026102300010043ULL', '0x2026102400010052ULL',
            '0x2026102400010053ULL', '0x2026102400010054ULL', '0x2026102400010056ULL'):
    check(prefix, {**valid, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
check(prefix, {'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': valid['PIONEER_DRIVER_BUILD_TAG']}, False)
for name in ('PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_GDN_RECURRENT_VALIDATION_NATIVE',
             'PIONEER_Q38_GDN_INPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE',
             'PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_QUALIFICATION',
             'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
             'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
    for value in ('0', '1'):
        check(prefix, {**valid, name: value}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput'):
    client = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    flags = {'PIONEER_Q38_'+client+'_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1'}
    if name != 'GdnOutput':
        flags['PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE'] = '1'
    include = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(include, flags, True)
    for value in ('0', '1', '2', '-1'):
        check(include, {**flags, 'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE': value}, False)

with tempfile.TemporaryDirectory(prefix='pioneer-output-debug-gates-') as temporary:
    root = pathlib.Path(temporary)
    for builder, runner in (('build_qwen38_gdn_output_validation_native.py', sys.executable),
                            ('build_qwen38_gdn_output_execution_native.py', sys.executable),
                            ('build_qwen38_gdn_rendezvous_validation_native.py', sys.executable),
                            ('build_qwen38_gdn_rendezvous_validation_client.sh', 'sh'),
                            ('build_qwen38_inference.sh', 'sh')):
        for flag in ('PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE', 'PIONEER_Q38_CPU_VALIDATION',
                     'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '', 'false'):
                result = subprocess.run([runner, str(directory/builder), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                        env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
    begin, end = '// PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE_ROUTING_BEGIN', '// PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE_ROUTING_END'
    assert source.count(begin) == source.count(end) == 1
    route = source.split(begin, 1)[1].split(end, 1)[0]
    allowed = ('TT_PCI_PROBE_Q38_BANK_SUPPORT', 'TT_PCI_PROBE_Q38_BANK_BEGIN', 'TT_PCI_PROBE_Q38_BANK_END',
               'TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT', 'TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD', 'TT_PCI_PROBE_Q38_SPAN_END')
    constants = '\n'.join(line for file in ('PioneerQ38BankProtocol.h', 'PioneerQ38SpanProtocol.h')
                          for line in (kernel/file).read_text().splitlines() if any(line.startswith('#define '+name+' ') for name in allowed))
    assert len(constants.splitlines()) == 6
    code = '#include <cassert>\n#include <cstddef>\n#define B_DEVICE_OP_CODES_END 1000\n'
    code += '#include "'+str(kernel/'PioneerQ38GdnOutputValidationProtocol.h')+'"\n'+constants+'\n'
    code += 'constexpr int B_NOT_ALLOWED=-11;unsigned calls=0,fallback=0;\n'
    code += 'int Q38GdnOutputValidationControl(void*,unsigned,void*,size_t){calls++;return 88;}\n'
    code += 'int Route(unsigned operation){void* instance=nullptr;void* data=nullptr;size_t length=0;\n'+route+'\nfallback++;return 5;}\n'
    code += 'int main(){for(unsigned op=0;op<131072;op++){int result=Route(op);\n'
    code += 'if(op==TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATE || op==TT_PCI_PROBE_Q38_GDN_OUTPUT_VALIDATION_SUPPORT)assert(result==88);\n'
    code += 'else if('+ ' || '.join('op=='+name for name in allowed)+')assert(result==5);else assert(result==B_NOT_ALLOWED);}\n'
    code += 'assert(calls==2 && fallback==6);static_assert(sizeof(tt_q38_gdn_output_validation)==3352320);}\n'
    path, binary = root/'routes.cpp', root/'routes'
    path.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path), '-o', str(binary)], check=True, env=clean)
    subprocess.run([str(binary)], check=True, env=clean)
print(f'PASS DEBUG55 exact role/tag/native parents/old namespace+CPU separation/builder gates{count}/3352320B ABI/131072 ACTUAL closed routes;2controls+6bank/span only,NO Work/numeric input/address grant')
