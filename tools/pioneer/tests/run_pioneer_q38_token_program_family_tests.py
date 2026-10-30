#!/usr/bin/env python3
"""QTP1/QTP2 complete-program DATA selection;no native or device execution."""
import os
import pathlib
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parents[1]
macro = 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS'
programs = '#include "'+str(here/'PioneerQ38TokenPrograms.h')+'"\n'
selected = '#include "'+str(here/'PioneerQ38SelectedGlobalWorker.h')+'"\n'
footprint = '#include "'+str(here/'PioneerQ38TokenFootprint.h')+'"\n'
role = '#include "'+str(here/'PioneerQ38TokenPreciseProgramsRole.h')+'"\n'
accepted = refused = 0


def check(source, okay):
    global accepted, refused
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-fsyntax-only', '-x', 'c++', '-'],
                            input='#include <stdint.h>\n'+source, text=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == okay, (source, result.stderr)
    if okay:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


for precise in (False, True):
    define = '#define '+macro+' 1\n' if precise else ''
    assertions = ('static_assert(PioneerQ38TokenPrograms::Family=='+hex(0x32505451 if precise else 0x31505451)+');\n'
                  'static_assert(PioneerQ38TokenL1::Precise=='+str(int(precise))+');\n'
                  'static_assert(PioneerQ38SelectedWorkerPrecise=='+str(int(precise))+');\n'
                  'static_assert(PioneerQ38SelectedWorkerBootEnd=='+str(49 if precise else 46)+');\n'
                  'static_assert(PioneerQ38SelectedGlobalWorker::Magic=='+hex(0x32574751 if precise else 0x31574751)+');\n'
                  'static_assert(PioneerQ38TokenL1::SavedWords=='+str(13356602 if precise else 11406906)+');\n')
    check(define+programs+selected+assertions+programs+selected+footprint, True)
    # Kernel DATA is not a genuine native context. It supplies no IO hook,
    # driver entry point,ownership,loaded-model or launch permission.
    check('#define _KERNEL_MODE 1\n'+define+programs+selected+assertions, True)
    change = '#undef '+macro+'\n' if precise else '#define '+macro+' 1\n'
    for first, second in ((programs, programs), (footprint, footprint), (selected, selected), (footprint, programs)):
        check(define+first+change+second, False)
for value in (None, 0, 2):
    define = '' if value is None else '#define '+macro+' '+str(value)+'\n'
    check(define+role, False)
    check(define+'#include "'+str(here/'PioneerQ38TokenPrecisePrograms.h')+'"\n', False)
for name in ('PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION', 'PIONEER_Q38_PRECISE_WORKER_IMAGE',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
             'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT'):
    for value in (0, 1):
        define = '#define '+macro+' 1\n'
        contamination = '#define '+name+' '+str(value)+'\n'
        check(define+contamination+programs, False)
        check(define+programs+contamination+programs, False)
for header, flags in (
        ('PioneerQ38GlobalWorkerImageRole.h', {'PIONEER_Q38_GLOBAL_WORKER_IMAGE': 1}),
        ('PioneerQ38PreciseWorkerImageRole.h', {'PIONEER_Q38_PRECISE_WORKER_IMAGE': 1}),
        ('PioneerQ38TokenImageRole.h', {'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE': 1,
                                      'PIONEER_Q38_GLOBAL_CHAIN_FEEDBACK': 1}),
        ('PioneerQ38GlobalChainIsolation.h', {})):
    prefix = ''.join('#define '+name+' '+str(value)+'\n' for name, value in flags.items())
    include = '#include "'+str(here/header)+'"\n'
    for value in (0, 1):
        contamination = '#define '+macro+' '+str(value)+'\n'
        check(prefix+contamination+include, False)
        check(prefix+include+contamination+include, False)
print(f'PASS complete-program DATA family selection/positive{accepted}/refusals{refused}/QTP1+QTP2 boot+footprint+ABI exact/late+zero+two+native/image mixing denied;NO native or device grant')
