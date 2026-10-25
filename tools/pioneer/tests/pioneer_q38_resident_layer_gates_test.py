#!/usr/bin/env python3
"""New layer roles,exact ROM bytes,late CPU aliases,closed legacy entries."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_q38_resident_layer_image_common import PROGRAM_PINS, verify

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
image, other = map(pathlib.Path, sys.argv[1:])
assert image.resolve() != other.resolve(), 'Two independent build directories required'
for root in (image, other):
    verify(root, root/'unchanged-output/exact-companion')
    assert not (root/'program.undefined').read_bytes()
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (root/'program.symbols').read_text(), re.M)
    rows = (root/'program.sections').read_text().splitlines()
    for i, row in enumerate(rows):
        match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
        if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
            assert match[1][1:] in {p[0] for p in PROGRAM_PINS}
    csr = []
    for line in (root/'program.disassembly').read_text().splitlines():
        match = re.match(r'\s*([0-9a-f]+):\s+([0-9a-f]{8})\s', line)
        if match:
            at, word = int(match[1], 16), int(match[2], 16)
            if word & 127 == 0x73:
                assert (word >> 12) & 7 == 2 and (word >> 15) & 31 == 0 and (word >> 7) & 31
                csr.append((at, word >> 20))
    assert [v for _, v in csr] == [0xb80, 0xb00, 0xb80]*2
    assert all(0x68000 <= a < 0x6e000 for a, _ in csr[:3]) and all(0x15c000 <= a < 0x17e000 for a, _ in csr[3:])
flags = ('PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE', 'PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE', 'PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE')
root = dict(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE='1', PIONEER_Q38_FFN_PERSISTENT='1', PIONEER_Q38_PROJECTION_PERSISTENT='1',
            PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE='1', PIONEER_Q38_RESIDENT_OUTPUT_ROOT_IMAGE='1', **{flags[0]:'1'})
norm = {flags[1]:'1'}
ffn = dict(PIONEER_Q38_FFN_GRAPH_ROOT='1', PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT='1', **{flags[2]:'1'})
roles = (
    ('PioneerQ38ResidentLayerRootImageRole.h', root),
    ('blackhole_qwen38_resident_layer_norm.cpp', norm),
    ('PioneerQ38ResidentLayerFfnImageRole.h', ffn),
)
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx, count = os.environ.get('CXX', 'c++'), 0


def check(text, definitions, expected):
    global count
    result = subprocess.run([cxx, '-std=c++17', '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in definitions.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=10)
    assert (result.returncode == 0) == expected, (definitions, result.stderr)
    count += 1


for name, valid in roles:
    text = '#include "'+str(directory/name)+'"\n'
    check(text, valid, True)
    for key in valid:
        for value in (None, '0', '2', '-1', ''):
            definitions = dict(valid)
            if value is None:
                del definitions[key]
            else:
                definitions[key] = value
            check(text, definitions, False)
        check(text+'#undef '+key+'\n'+text, valid, False)
    for key in ('PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', '_KERNEL_MODE', '__FAST_MATH__',
                'PIONEER_Q38_RESIDENT_OUTPUT_OWNER', 'PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE',
                'PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY', 'PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_CLIENT',
                *[f for f in flags if f not in valid]):
        for value in ('0', '1'):
            check(text, {**valid, key:value}, False)
            check(text+'#define '+key+' '+value+'\n'+text, valid, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    kind = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    cpu = {'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT':'1', 'PIONEER_Q38_CPU_VALIDATION':'1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    if name == 'NextGdnOutput':
        cpu = dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1', PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1')
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, cpu, True)
    for flag in flags:
        for value in ('0', '1'):
            check(text, {**cpu, flag:value}, False)
            check(text+'#define '+flag+' '+value+'\n'+text, cpu, False)
prefix = (kernel/'tenstorrent_pci_probe.cpp').read_text().split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
check(prefix, {}, True)
session = '#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
session_cpu = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1', PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
check(session, session_cpu, True)
for flag in flags:
    for value in ('0', '1'):
        check(session, {**session_cpu, flag:value}, False)
        check(session+'#define '+flag+' '+value+'\n'+session, session_cpu, False)
        check(prefix, {flag:value}, False)
        check(prefix, {flag:value, 'TT_PIONEER_DRIVER_RELOAD':'1', 'PIONEER_DRIVER_BUILD_TAG':'0x2026102100010012ULL'}, False)
        result = subprocess.run([cxx, '-std=c++17', '-E', '-D'+flag+'='+value, str(directory/'pioneer_q38_original_source.cpp')],
                                text=True, capture_output=True, env=clean, timeout=30)
        assert result.returncode != 0 and 'Complete resident layer IMAGE never contains CPU reference observers' in result.stderr
        count += 1
with tempfile.TemporaryDirectory(prefix='pioneer-layer-gates-', dir='/private/tmp') as temporary:
    temp = pathlib.Path(temporary)
    for flag in flags:
        for value in ('0', '1', '2', '-1', '', 'false', '0\n1'):
            for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
                argv = [str(temp/'NO_SDK'), *([str(temp/'NO_LIB')] if name == 'inference' else []), str(temp/'NO_OUTPUT')]
                result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True,
                                        env={**clean, flag:value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (temp/'NO_OUTPUT').exists()
                count += 1
    for flag in (*flags, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run([sys.executable, str(directory/'build_qwen38_resident_layer_image.py'), str(temp/'NO_SDK'), str(temp/'NO_OUTPUT')],
                                    capture_output=True, text=True, env={**clean, flag:value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (temp/'NO_OUTPUT').exists()
            count += 1
print(f'PASS NEW complete-layer gates{count}/TWO independently built7section ROOT+same6sectionpeer bytes/read-only clocks/exact0+late CPU/native/Work aliases rejected/ALL legacy entries inert;NO native owner,physical,Work,all64layer or token-rate grant')
