#!/usr/bin/env python3
"""Actual private hidden read methods;all lifecycle/source/NoC endpoints MOCK."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
owner = (kernel/'PioneerQ38TokenNativeOwner.h').read_text()
hidden = (kernel/'PioneerQ38TokenHiddenRead.inc').read_text()

def extract(source, signature):
    assert source.count(signature) == 1, signature
    begin = source.index(signature)
    brace = source.index('{', begin)
    depth, end = 1, brace+1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}')
        end += 1
    return source[begin:end]

signatures = (
    (owner, 'template<class IO> bool BootTime('),
    (owner, 'template<class IO,class Result> bool HoldGuard('),
    (owner, 'template<class IO> bool ReadIdle('),
    (owner, 'template<class IO> bool ReadFrontier('),
    (hidden, 'bool HiddenReadOutput('),
    (hidden, 'template<class IO> bool ReadHiddenPacket('),
    (hidden, 'template<class IO> __attribute__((noinline)) bool ReadHiddenChunk('),
)
methods = [extract(source, signature) for source, signature in signatures]
records = '\n'.join(extract(owner, 'struct '+name+' {')+';' for name in (
    'PioneerQ38TokenReadResult', 'PioneerQ38TokenHiddenReadResult'))
with tempfile.TemporaryDirectory(prefix='pioneer-hidden-read-method-', dir='/private/tmp') as temporary:
    tmp = pathlib.Path(temporary)
    (tmp/'actual_hidden_records.inc').write_text(records+'\n')
    (tmp/'actual_hidden_methods.inc').write_text('\n'.join(methods)+'\n')
    binary = tmp/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(tmp), str(here/'pioneer_q38_hidden_read_method_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
for (_, signature), method in zip(signatures, methods):
    print('ACTUAL_HIDDEN_METHOD_SHA256 '+signature+hashlib.sha256(method.encode()).hexdigest())
print('EXTRACTED METHODS ONLY;ALL source/lifecycle/terminal/NoC endpoints are MOCK. '
      'NO genuine model/processor/native owner/physical/numerical/Work/TPS qualification.')
