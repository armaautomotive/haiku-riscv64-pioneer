#!/usr/bin/env python3
"""Exact CPU DEBUG55 and original-reference chain,never native or Work."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
cxx = os.environ.get('CXX', 'c++')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
valid = {'PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1',
         'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
session = '#include "'+str(directory/'PioneerQ38GdnOutputValidationSession.h')+'"\n'
count = 0


def check(text, flags, expected):
    global count
    result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=5)
    assert (result.returncode == 0) == expected, (flags, result.stderr)
    count += 1


check(session, valid, True)
check(session, {}, False)
for flag in valid:
    for value in (None, '0', '2', '-1'):
        flags = dict(valid)
        if value is None:
            del flags[flag]
        else:
            flags[flag] = value
        check(session, flags, False)
for name in ('Embedding', 'LearnedRms', 'Projection', 'ResidentDecoder', 'GdnRecurrent', 'GdnRendezvous', 'GdnOutput'):
    include = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(include, valid, True)
check('#include "'+str(directory/'PioneerQ38GdnInputValidationReference.h')+'"\n', valid, False)  # old scaled-Q namespace stays separate
reference = (directory/'PioneerQ38GdnOutputValidationReference.h').read_text()
forbidden = sorted(set(re.findall(r'defined\(([^)]+)\)', reference.split('#ifndef PIONEER_Q38_GDN_OUTPUT_VALIDATION_REFERENCE_H', 1)[0])) - set(valid))
for flag in forbidden:
    for value in ('0', '1'):
        check(session, {**valid, flag: value}, False)
for flag in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE',
             'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT'):
    for value in ('0', '1'):
        check(session+'#define '+flag+' '+value+'\n'+session, valid, False)
check(session+'#undef PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE\n#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 0\n'+session, valid, False)
with tempfile.TemporaryDirectory(prefix='pioneer-output-client-gates-') as temporary:
    root = pathlib.Path(temporary)
    for name in ('build_qwen38_gdn_output_validation_client.sh', 'build_qwen38_gdn_rendezvous_validation_client.sh'):
        for flag in (*valid, 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '', 'false'):
                result = subprocess.run(['sh', str(directory/name), str(root/'NO_SDK'), str(root/'NO_OUTPUT')],
                                        env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists(), (name, flag, value, result.returncode, result.stderr)
                count += 1
    # Valid argument counts: prove rejection is the new diagnostic-mode
    # gate BEFORE attempting to resolve an SDK/library,not missing argv.
    for name, arguments in (
            ('build_qwen38_inference.sh', [str(root/'NO_SDK'), str(root/'NO_LIBRARY'), str(root/'NO_OUTPUT')]),
            ('build_qwen38_banks_client.sh', [str(root/'NO_SDK'), str(root/'NO_OUTPUT')])):
        for flag in ('PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT', 'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE'):
            for value in ('1', '2', '', 'false'):
                result = subprocess.run(['sh', str(directory/name), *arguments], env={**clean, flag: value},
                                        capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root/'NO_OUTPUT').exists(), (name, flag, value, result.returncode, result.stderr)
                count += 1
        for value in (None, '0'):
            flags = {} if value is None else {'PIONEER_Q38_GDN_OUTPUT_VALIDATION_CLIENT': value,
                                              'PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE': value}
            result = subprocess.run(['sh', str(directory/name), *arguments], env={**clean, **flags},
                                    capture_output=True, text=True, timeout=5)
            assert result.returncode == 1 and not result.stdout and 'NO_SDK' in result.stderr and not (root/'NO_OUTPUT').exists(), (name, value, result.returncode, result.stderr)
            count += 1  # control actually reaches SDK lookup;diagnostic flags caused earlier refusals
print(f'PASS isolated CPU DEBUG55 client/exact reference chain/mixed+defined-zero/reinclude/builder gates{count};old scaled-Q helper denied,NO native/Work/CPU product uploads')
