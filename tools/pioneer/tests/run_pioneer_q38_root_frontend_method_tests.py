#!/usr/bin/env python3
"""Actual ROOT observer/seal methods; endpoint,grammar and authority MOCK only."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
current = (root/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h').read_text()
memory = (root/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenMemory.h').read_text()
old = (here/'pioneer_q38_root_frontend_before_fix.inc').read_text()
assert hashlib.sha256(old.encode()).hexdigest() == '8586c186e69ca6b6adbe58db8b72dc7286e21ff40b8dd05defcb58655b212c56'
# Keep the actual 8-byte terminal code extent unchanged,including its exclusive end.
programs = (here.parent/'PioneerQ38TokenPushedPrograms.h').read_bytes()
assert hashlib.sha256(programs).hexdigest() == '692a5296c0bc3a19af30ffe920728834328acb05f2122c11c9f95abefd09256f'

def extract(source, signature):
    assert source.count(signature) == 1
    begin = source.index(signature)
    brace = source.index('{', begin)
    depth, end = 1, brace+1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}')
        end += 1
    return source[begin:end]

signatures = [
    'template<class IO> __attribute__((noinline)) bool RootBootSeal(',
    'template<class IO> __attribute__((noinline)) bool RootStopSeal(',
    'template<class IO> __attribute__((noinline)) bool InspectTerminal(',
    'template<class IO,class Result> bool HoldGuard(',
    'template<class IO> bool ObserveRoot(',
]
methods = [extract(current, signature) for signature in signatures]
baseline = extract(old, signatures[2])
assert baseline != methods[2]
records = '\n'.join(extract(current, 'struct '+name+' {')+';' for name in (
    'PioneerQ38TokenTerminalResult', 'PioneerQ38TokenHoldResult', 'PioneerQ38TokenExecutionResult'))
records += '\n'+extract(memory, 'struct PioneerQ38TokenMemoryResult {')+';'
with tempfile.TemporaryDirectory(prefix='pioneer-root-frontend-method-', dir='/private/tmp') as temporary:
    tmp = pathlib.Path(temporary)
    (tmp/'actual_root_records.inc').write_text(records+'\n')
    (tmp/'actual_root_methods.inc').write_text(
        baseline.replace('InspectTerminal(', 'OldInspectTerminal(', 1)+'\n'+'\n'.join(methods)+'\n')
    binary = tmp/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(tmp), str(here/'pioneer_q38_root_frontend_method_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
for signature, method in zip(signatures, methods):
    print('ACTUAL_ROOT_METHOD_SHA256 '+signature.split('bool ')[1]+hashlib.sha256(method.encode()).hexdigest())
print('EXTRACTED METHODS ONLY;endpoint,grammar,source authority and image verifier are MOCK. '
      'NO genuine model/processor history,native owner,physical completion,Work or TPS proof.')
