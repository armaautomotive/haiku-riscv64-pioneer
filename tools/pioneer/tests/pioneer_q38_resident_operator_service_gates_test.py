#!/usr/bin/env python3
"""Service IMAGE closed roles/builders and complete emitted layout;NO device."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
images = list(map(pathlib.Path, sys.argv[1:]))
for section in ('boot', 'text', 'affine', 'ffn', 'service'):
    assert (images[0]/(section+'.bin')).read_bytes() == (images[1]/(section+'.bin')).read_bytes()
for image in images:
    assert not (image/'program.undefined').read_bytes()
    assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub)\b', (image/'program.disassembly').read_text())
    assert '0x00068000' in (image/'program.map').read_text() and '0x00072000' in (image/'program.map').read_text()
valid = dict(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE='1', PIONEER_Q38_FFN_PERSISTENT='1', PIONEER_Q38_PROJECTION_PERSISTENT='1')
role = '#include "'+str(directory/'PioneerQ38ResidentOperatorImageRole.h')+'"\n'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx, count = os.environ.get('CXX', 'c++'), 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(role, valid, True)
for name in valid:
    for value in (None, '0', '2', '-1', ''):
        flags = dict(valid)
        if value is None:
            del flags[name]
        else:
            flags[name] = value
        check(role, flags, False)
        check(role+'#undef '+name+'\n'+role, valid, False)
# Derive EVERY explicitly forbidden role from the actual role header,then
# execute preprocessor refusals and late-role reinclusion,including zero.
for name in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)', (directory/'PioneerQ38ResidentOperatorImageRole.h').read_text()))-set(valid)):
    for value in ('0', '1'):
        check(role, {**valid, name:value}, False)
        check(role+'#define '+name+' '+value+'\n'+role, valid, False)
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
check(prefix, {}, True)
for value in ('0', '1'):
    check(prefix, {'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE':value}, False)
    check(prefix, {'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE':value, 'TT_PIONEER_DRIVER_RELOAD':'1', 'PIONEER_DRIVER_BUILD_TAG':'0x2026102100010012ULL'}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent',
             'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    kind = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    cpu = {'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT':'1', 'PIONEER_Q38_CPU_VALIDATION':'1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    if name == 'NextGdnOutput':
        cpu = dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1', PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1')
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, cpu, True)
    for value in ('0', '1'):
        check(text, {**cpu, 'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE':value}, False)
        check(text+'#define PIONEER_Q38_RESIDENT_OPERATOR_IMAGE '+value+'\n'+text, cpu, False)
session = '#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
cpu = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1', PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
for value in ('0', '1'):
    check(session, {**cpu, 'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE':value}, False)
    check(session+'#define PIONEER_Q38_RESIDENT_OPERATOR_IMAGE '+value+'\n'+session, cpu, False)
    result = subprocess.run([cxx, '-std=c++17', '-E', '-DPIONEER_Q38_RESIDENT_OPERATOR_IMAGE='+value,
                             str(here/'pioneer_q38_decoder_original_source.cpp')], capture_output=True, text=True, env=clean, timeout=30)
    assert result.returncode != 0 and 'Original-model observer remains separate from resident operator IMAGE' in result.stderr
    count += 1
with tempfile.TemporaryDirectory(prefix='pioneer-resident-service-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True,
                                    env={**clean, 'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE':value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    for name in (*valid, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run([sys.executable, str(directory/'build_qwen38_resident_operator_service_image.py'),
                                     str(root/'NO_SDK'), str(root/'NO_OUTPUT')], capture_output=True, text=True,
                                    env={**clean, name:value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
print(f'PASS resident operator IMAGE roles/reinclude/defined-zero/CPU+native+Work exclusion/builder gates{count}/TWO complete emitted images identical/no unsupported instructions;NO native owner/old snapshot/model/card speed grant')
for section in ('boot', 'text', 'affine', 'ffn', 'service'):
    print(hashlib.sha256((images[0]/(section+'.bin')).read_bytes()).hexdigest()+' '+section)
