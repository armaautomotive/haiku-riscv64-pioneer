#!/usr/bin/env python3
"""Optimized full-audit ROOT pins/export/relocation/builders;NO device."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
directory = here.parent
images = list(map(pathlib.Path, sys.argv[1:]))
saved = list(sys.argv)
try:
    sys.argv = [str(here/'pioneer_q38_resident_cohort_gates_test.py'),
                *[str(p/'unchanged-full-audit') for p in images]]
    prior = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
pins = (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 92, '874faf837e78bacfb006eba25bb85bec23ac3ea284a7f0f8972fd509ae12b974'),
    ('conv', 6472, '19d7092a22366e405987251b2b25447083d6a34022d271398a9201ae5c265498'),
    ('cohort', 36880, '0c8dc8357c0381167d063e7e8de03afcebcff33dc3aa6699ccfac6268e1d056b'),
)
for image in images:
    for section, size, digest in pins:
        data = (image/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    assert not (image/'program.undefined').read_bytes()
    assert not re.search(r'^[0-9a-f]+ [bBdDgGsS] ', (image/'program.symbols').read_text(), re.M)
    assert not re.search(r'\b(?:divu?|remu?|mulh[su]*|fdiv\.[sd]|fsqrt\.[sd]|fmadd|fmsub|fnmadd|fnmsub|fcvt\.wu?\.s|csr\w*)\b', (image/'program.disassembly').read_text())
    rows = (image/'program.sections').read_text().splitlines()
    for i, row in enumerate(rows):
        match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+', row)
        if match and i+1 < len(rows) and 'ALLOC' in rows[i+1]:
            assert match[1] in ('.boot', '.text', '.conv', '.cohort')
    text = (image/'text.bin').read_bytes()
    old = (image/'unchanged-full-audit/text.bin').read_bytes()
    assert text[:4] == old[:4] and text[12:] == old[12:] and len(text) == len(old) == 92
    auipc, jalr = struct.unpack('<II', text[4:12])
    assert auipc & 0xfff == 0x097 and jalr & 0xfffff == 0x080e7
    immediate = jalr >> 20
    if immediate & 0x800:
        immediate -= 0x1000
    target = (0x72004+(auipc & 0xfffff000)+immediate) & 0xffffffff
    assert re.search('^'+format(target, '08x')+' T q38_resident_cohort_main$', (image/'program.symbols').read_text(), re.M)
assert (images[0]/'unchanged-full-audit/root_conv.o').read_bytes() == (images[1]/'unchanged-full-audit/root_conv.o').read_bytes()
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count, cxx = 0, os.environ.get('CXX', 'c++')
source = '#include "'+str(directory/'blackhole_qwen38_resident_cohort.cpp')+'"\n'
valid = prior['valid']
for value in (None, '1', '0', '2', '-1', ''):
    flags = dict(valid)
    if value is not None:
        flags['PIONEER_Q38_COHORT_POLL_BACKOFF'] = value
    result = subprocess.run([cxx, '-E', '-x', 'c++', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=source, capture_output=True, text=True, env=clean, timeout=10)
    assert (result.returncode == 0) == (value in (None, '1')), result.stderr
    count += 1
with tempfile.TemporaryDirectory(prefix='pioneer-cohort-optimized-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    builder = str(directory/'build_qwen38_resident_cohort_optimized_image.py')
    for name in (*valid, 'PIONEER_Q38_COHORT_POLL_BACKOFF', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '0\n1'):
            result = subprocess.run([sys.executable, builder, str(root/'NO_SDK'), str(root/'NO_OUTPUT'), '--O3', '--bounded-backoff'],
                                    capture_output=True, text=True, env={**clean, name:value}, timeout=5)
            assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
    for options in (['--O0'], ['--O4'], ['--O3', '--unbounded'], ['--O3', '--bounded-backoff', 'extra'], ['--bounded-backoff']):
        result = subprocess.run([sys.executable, builder, str(root/'NO_SDK'), str(root/'NO_OUTPUT'), *options],
                                capture_output=True, text=True, env=clean, timeout=5)
        assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists()
        count += 1
print(f'PASS optimized full-audit ROOT extra gates{count}/prior464roles/TWO exact images/ALL-ALLOC exports/exact frontend relocation/original numerical object/strict bounded backoff+builders;NO native/Work/fullmodel/card speed grant')
for section, size, digest in pins:
    print(digest+' '+section+' bytes'+str(size))
