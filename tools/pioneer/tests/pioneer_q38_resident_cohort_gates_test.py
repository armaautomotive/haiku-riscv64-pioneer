#!/usr/bin/env python3
"""Complete cohort ROOT immutable image and closed entry points;NO card."""
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
pins = (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 92, 'ce66ec25896f1e06362bae978499b33222c7c51d603ce36e83bf0e077b804c22'),
    ('conv', 6472, 'feb352acfc6490b727e32ba236d8bfa2e9f7b8811b31ae4ee37f5e4d53624112'),
    ('cohort', 14412, 'dc7c83eafe4f64ad03137ce6889abc9a5d27e5d3f25865520d960098f3d06b07'),
)
for image in images:
    for section, size, digest in pins:
        data = (image/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    assert not (image/'program.undefined').read_bytes()
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (image/'program.symbols').read_text(), re.M)
    assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|csr\w*)\b', (image/'program.disassembly').read_text())
    assert '0x000b8000' in (image/'program.map').read_text() and '.q38_projection_data' in (image/'program.map').read_text()
valid = dict(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE='1', PIONEER_Q38_FFN_PERSISTENT='1',
             PIONEER_Q38_PROJECTION_PERSISTENT='1', PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE='1')
role = '#include "'+str(directory/'PioneerQ38ResidentCohortImageRole.h')+'"\n'
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
headers = ('PioneerQ38ResidentOperatorImageRole.h', 'PioneerQ38ResidentCohortImageRole.h')
for name in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)', ''.join((directory/p).read_text() for p in headers)))-set(valid)):
    for value in ('0', '1'):
        check(role, {**valid, name:value}, False)
        check(role+'#define '+name+' '+value+'\n'+role, valid, False)
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
check(prefix, {}, True)
flag = 'PIONEER_Q38_RESIDENT_COHORT_ROOT_IMAGE'
for value in ('0', '1'):
    check(prefix, {flag:value}, False)
    check(prefix, {flag:value, 'TT_PIONEER_DRIVER_RELOAD':'1', 'PIONEER_DRIVER_BUILD_TAG':'0x2026102100010012ULL'}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent',
             'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    kind = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    cpu = {'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT':'1', 'PIONEER_Q38_CPU_VALIDATION':'1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    if name == 'NextGdnOutput':
        cpu = dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1', PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1')
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, cpu, True)
    for value in ('0', '1'):
        check(text, {**cpu, flag:value}, False)
        check(text+'#define '+flag+' '+value+'\n'+text, cpu, False)
session = '#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
cpu = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1', PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
for value in ('0', '1'):
    check(session, {**cpu, flag:value}, False)
    check(session+'#define '+flag+' '+value+'\n'+session, cpu, False)
    result = subprocess.run([cxx, '-std=c++17', '-E', '-D'+flag+'='+value,
                             str(here/'pioneer_q38_decoder_original_source.cpp')], capture_output=True, text=True, env=clean, timeout=30)
    assert result.returncode != 0 and 'Original-model observer remains separate from resident operator IMAGE' in result.stderr
    count += 1
with tempfile.TemporaryDirectory(prefix='pioneer-resident-cohort-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True,
                                    env={**clean, flag:value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    for builder in ('resident_operator_service', 'resident_gdn_operator_service', 'resident_cohort'):
        for name in (*valid, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '-1', '', 'false', '0\n1'):
                result = subprocess.run([sys.executable, str(directory/('build_qwen38_'+builder+'_image.py')),
                                         str(root/'NO_SDK'), str(root/'NO_OUTPUT')], capture_output=True, text=True,
                                        env={**clean, name:value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
print(f'PASS complete ROOT IMAGE gates{count}/TWO exact images/CPU+native+Work closed/NEW explicit immutable export+manifest footprint;NO old native snapshot/initialized owner/current-slot/all64layers/token-loop or physical speed grant')
for section, size, digest in pins:
    print(digest+' '+section+' bytes'+str(size))
