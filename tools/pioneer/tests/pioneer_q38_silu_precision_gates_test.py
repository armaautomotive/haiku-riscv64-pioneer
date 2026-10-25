#!/usr/bin/env python3
"""LOCAL-only SiLU image, role, builder, observer and byte regressions."""
import hashlib
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
images = [pathlib.Path(p) for p in sys.argv[1:]]
pin = 'c9d917403ae39611d262b1f97aa0bf44e23c21661fec33b6f9a3739493ef08f6'
for image in images:
    code = (image/'code.bin').read_bytes()
    assert len(code) == 3152 and hashlib.sha256(code).hexdigest() == pin
    assert (image/'boot.bin').read_bytes() == struct.pack('<I', 0x0005e06f)
    assert not (image/'program.undefined').read_bytes()
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (image/'program.symbols').read_text(), re.M)
    frames = [line.split('\t') for line in (image/'program.su').read_text().splitlines()]
    assert frames and max(int(row[1]) for row in frames) == 384 and all(row[2] == 'static' for row in frames)
    disassembly = (image/'program.disassembly').read_text()
    assert not re.search(r'\b(?:fdiv\.[sd]|fsqrt\.[sd]|f(?:add|sub|mul)\.d|divu?|remu?|mulh[su]*|fmadd|fmsub|fnmadd|fnmsub)\b', disassembly)
assert (images[0]/'code.bin').read_bytes() == (images[1]/'code.bin').read_bytes()
roles = ('PIONEER_Q38_SILU_PRECISION_IMAGE', 'PIONEER_Q38_SILU_LEGACY_CONTROL_IMAGE')
isolation = directory/'PioneerQ38SiluIsolation.h'
header = '#include "'+str(isolation)+'"\n'
precision = '#include "'+str(directory/'PioneerQ38SiluPrecision.h')+'"\n'
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
count = 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(header, {}, False)
check(precision, {roles[0]: '1'}, True)
check(precision, {roles[1]: '1'}, False)
for role in roles:
    check(header, {role: '1'}, True)
    for value in ('0', '2', '-1', ''):
        check(header, {role: value}, False)
    for other in roles:
        if other != role:
            for value in ('0', '1'):
                check(header, {role: '1', other: value}, False)
    forbidden = sorted(set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', isolation.read_text()))-set(roles))
    assert len(forbidden) >= 40
    for flag in forbidden:
        for value in ('0', '1'):
            check(header, {role: '1', flag: value}, False)
            check(header+'#define '+flag+' '+value+'\n'+header, {role: '1'}, False)
            if role == roles[0]:
                check(precision+'#define '+flag+' '+value+'\n'+precision, {role: '1'}, False)
driver = (kernel/'tenstorrent_pci_probe.cpp').read_text().split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE', 1)[0].replace('#include "tenstorrent_pci_probe.h"', '')
check(driver, {}, True)
for role in roles:
    for value in ('0', '1', '2', '-1'):
        check(driver, {role: value}, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnInput', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput', 'NextProjection', 'NextGdnOutput'):
    client = 'GDN_OUTPUT' if name == 'GdnOutput' else 'GDN_RENDEZVOUS' if name == 'GdnRendezvous' else 'GDN_RECURRENT' if name == 'GdnRecurrent' else 'GDN_INPUT'
    flags = {'PIONEER_Q38_'+client+'_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
    if name == 'NextGdnOutput':
        flags = {'PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE': '1', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1'}
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text, flags, True)
    for role in roles:
        for value in ('0', '1'):
            check(text, {**flags, role: value}, False)
            check(text+'#define '+role+' '+value+'\n'+text, flags, False)
with tempfile.TemporaryDirectory(prefix='pioneer-silu-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('silu_precision_image', 'silu_legacy_control_image', 'gdn_prepare_unscaled_image'):
        for role in (*roles, 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '', 'false', '0\n1'):
                result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                        capture_output=True, text=True, env={**clean, role: value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image', 'ffn_matrix_image', 'ffn_persistent_image', 'projection_graph_image', 'projection_matrix_image', 'projection_persistent_image'):
        argv = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name == 'inference' else []), str(root/'NO_OUTPUT')]
        baseline = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True, env=clean, timeout=5)
        assert baseline.returncode and not baseline.stdout and not (root/'NO_OUTPUT').exists()
        for role in roles:
            for value in ('1', '2', '-1', '', 'false', '0\n1'):
                result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True, env={**clean, role: value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
                count += 1
            result = subprocess.run(['sh', str(directory/('build_qwen38_'+name+'.sh')), *argv], capture_output=True, text=True, env={**clean, role: '0'}, timeout=5)
            assert (result.returncode, result.stdout, result.stderr) == (baseline.returncode, baseline.stdout, baseline.stderr)
            count += 1
print(f'PASS LOCAL QSP1 precision gates{count}/two3152B images EXACT{pin[:8]}/384Bmaxframe/no helpers or unsupported instructions/native+CPU observers+reinclude+Work+builder isolation/old neutral refusal EXACT;NO physical pass or speed grant')
