#!/usr/bin/env python3
"""Actual native prefix/CPU exclusion/ambient image selectors refuse QPS2."""
import os
import pathlib
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_NEXT_RMS_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
flag = 'PIONEER_Q38_PROJECTION_NEXT_ROOT'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
count = 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(prefix, {}, True)
for value in ('0', '1', '2', '-1', ''):
    check(prefix, {flag: value}, False)
    check(prefix, {flag: value, 'PIONEER_Q38_WORK_INFERENCE': '1'}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput'):
    client = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    flags = {'PIONEER_Q38_'+client+'_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, flags, True)
    for value in ('0', '1', '2', '-1'):
        check(text, {**flags, flag: value}, False)
    for value in ('0', '1'):
        check(text+'#define '+flag+' '+value+'\n'+text, flags, False)
alias = '#include "'+str(kernel/'PioneerQ38NextProjectionImageAliases.h')+'"\n'
check(alias, {flag: '1'}, True)
for value in ('0', '2', '-1', ''):
    check(alias, {flag: value}, False)
for value in ('0', '2'):
    check(alias+'#undef '+flag+'\n#define '+flag+' '+value+'\n'+alias, {flag: '1'}, False)
for foreign in ('PIONEER_Q38_PROJECTION_RESULT_STASH', 'PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT'):
    for value in ('0', '1'):
        check(alias, {flag: '1', foreign: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-qps2-gates-') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
        for value in ('0', '1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv],
                                    capture_output=True, text=True, env={**clean, flag: value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists(), (name, value, result.stderr)
            count += 1
print(f'PASS QPS2 exact image-only flag/native+Work reject/reference+reinclude/defined-zero/mixed namespaces/10ambient legacy builder gates={count};NO public/native grant')
