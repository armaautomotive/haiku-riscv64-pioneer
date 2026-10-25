#!/usr/bin/env python3
"""All-layer returning output IMAGE isolation and exact emitted bytes."""
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
pins = (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 7940, 'd726337fe94d7d94812aaf5e67f3d4e328a21873cb27aa982dc5315bb6294b07'),
    ('affine', 5360, 'fcdf8ec5b607550db93fda5a86830951e2518d5580b11f4538fb6eead0eb3b7b'),
    ('ffn', 16352, 'f17def1072f3e9ceaff2a7f1f66596ca34efc4d7ed9e3ea0d52dbc4cf9a7acb9'),
    ('service', 10300, 'd00147de57e51e096760da0e6d25476692646807bcb0dc52d5a02b1263657254'),
    ('gdn', 35396, '36458af185c3500d1cd1b1170672c2138910aff836bd8a6a407b07c68a80b93d'),
)
for image in map(pathlib.Path, sys.argv[1:]):
    for section, size, digest in pins:
        data = (image/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    assert not (image/'program.undefined').read_bytes()
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (image/'program.symbols').read_text(), re.M)
    assert re.search(r'^00072000 T block_entry$', (image/'program.symbols').read_text(), re.M)
    match = re.search(r'^([0-9a-f]+) T q38_resident_persistent_gdn_output$', (image/'program.symbols').read_text(), re.M)
    assert match and 0xb8000 <= int(match[1], 16) < 0xc8000
    rows = (image/'program.sections').read_text().splitlines()
    for i, row in enumerate(rows):
        match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
        if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
            assert match[1][1:] in {p[0] for p in pins}
    subprocess.run([sys.executable, str(here/'pioneer_q38_gdn_rendezvous_csr_gate.py'), str(image/'program.disassembly')], check=True)
valid = dict(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE='1', PIONEER_Q38_FFN_PERSISTENT='1',
             PIONEER_Q38_PROJECTION_PERSISTENT='1', PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE='1',
             PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE='1')
flag = 'PIONEER_Q38_RESIDENT_GDN_OUTPUT_OPERATOR_IMAGE'
role = '#include "'+str(directory/'PioneerQ38ResidentGdnOutputImageRole.h')+'"\n'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx, count = os.environ.get('CXX', 'c++'), 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-std=c++17', '-x', 'c++', '-E',
                             *['-D'+k+'='+v for k, v in flags.items()], '-'],
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
headers = ('PioneerQ38ResidentOperatorImageRole.h', 'PioneerQ38ResidentGdnOperatorImageRole.h', 'PioneerQ38ResidentGdnOutputImageRole.h')
for name in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)', ''.join((directory/p).read_text() for p in headers)))-set(valid)):
    for value in ('0', '1'):
        check(role, {**valid, name:value}, False)
        check(role+'#define '+name+' '+value+'\n'+role, valid, False)
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
check(prefix, {}, True)
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
with tempfile.TemporaryDirectory(prefix='pioneer-resident-gdn-output-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image',
                 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True,
                                    env={**clean, flag:value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    for name in (*valid, 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run([sys.executable, str(directory/'build_qwen38_resident_gdn_output_operator_service_image.py'),
                                     str(root/'NO_SDK'), str(root/'NO_OUTPUT')], capture_output=True, text=True,
                                    env={**clean, name:value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
print(f'PASS QOL1 gates{count}/TWO exact six-section images/ALL exported allocation/old control authenticated FIRST/kernel+10CPU refs+session+observer+10legacy builders closed/defined0+late aliases refused;NO native/fullmodel/speed grant')
