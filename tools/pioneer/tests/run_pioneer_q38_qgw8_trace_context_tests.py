#!/usr/bin/env python3
"""Exercise actual private context wrapper against synthetic objects only."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeExecution.inc'
raw = source.read_text(); signature = 'static __attribute__((noinline)) bool ReadQ38TokenNativeTraceChunk('
assert raw.count(signature) == 1
start = raw.index(signature); end = raw.index('{', start)+1; depth = 1
while depth:
    depth += (raw[end] == '{') - (raw[end] == '}'); end += 1
digest = hashlib.sha256(raw.encode()).hexdigest()
with tempfile.TemporaryDirectory(prefix='qgw8-trace-context-', dir='/private/tmp') as name:
    tmp = Path(name)
    (tmp/'actual_trace_context_wrapper.inc').write_text(raw[start:end]+'\n')
    binary = tmp/'test'
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(tmp), str(here/'pioneer_q38_qgw8_trace_context_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
print('EXTRACTED wrapper and synthetic endpoints only; no native or physical capability')
