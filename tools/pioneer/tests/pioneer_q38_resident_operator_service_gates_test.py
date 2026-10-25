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
with tempfile.TemporaryDirectory(prefix='pioneer-resident-service-gates-', dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
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
