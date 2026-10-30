#!/usr/bin/env python3
"""Synthetic physical-to-logical EOS state DATA tests; no model/device IO."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
compiler = os.environ.get('CXX', 'c++')
include = ''.join(f'#include "{here / name}"\n' for name in ('pioneer_q38_eos_state_capture.h','PioneerQ38EosStateDataBridge.h'))
base = '#define PIONEER_Q38_TEST_OBSERVED_TRACE 1\n'
cases = 0
def check(source, okay):
    global cases
    result = subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-x', 'c++', '-fsyntax-only', '-'], input=source, text=True, capture_output=True)
    assert (result.returncode == 0) == okay, result.stderr
    if not okay:
        assert any(p in result.stderr for p in ('Exact HOST', 'not kernel,image,native,CPU oracle or Work', 'HOST DATA only')), result.stderr
    cases += 1
check(base+include+'int main(){return 0;}\n', True)
for value in (None, 0, 2):
    check(('' if value is None else f'#define PIONEER_Q38_TEST_OBSERVED_TRACE {value}\n')+include, False)
for value in (0, 1):
    for late in (False, True):
        # Existing TT_WORK is an offline arithmetic policy only. This exact
        # HOST role never grants a real Work/native/hardware owner.
        check(base+(include if late else '')+f'#define TT_PIONEER_Q38_WORK_MODE {value}\n'+include, True)
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_ATTENTION_FREESTANDING',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE',
             'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT'):
    for value in (0, 1):
        for late in (False, True):
            check(base+(include if late else '')+f'#define {role} {value}\n'+include, False)
for role in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
             'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_ATTENTION_FREESTANDING',
             'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_GLOBAL_WORKER_IMAGE', 'PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE'):
    for value in (0, 1):
        result = subprocess.run([compiler, '-std=c++17', '-Werror', '-E', f'-D{role}={value}', str(here/'pioneer_q38_eos_state_data_bridge.cpp')], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        assert result.returncode and 'EOS state serializer implementation is HOST DATA only' in result.stderr, result.stderr
        cases += 1
print(f'PASS EOS capture/{cases} exact HOST/kernel+image+native+CPU-oracle+Work/defined0+1+late role gates;NO execution permission', flush=True)
with tempfile.TemporaryDirectory(prefix='pioneer-eos-state-capture-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    for legacy in (False, True):
        subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                        *(['-DTT_PIONEER_Q38_WORK_MODE=1'] if legacy else []), str(here/'pioneer_q38_eos_state_capture_test.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
        print(f'PASS capture HOST arithmetic legacyTT={int(legacy)};serializer implementation remains a separate forbidden-math TU', flush=True)
