#!/usr/bin/env python3
"""Extract actual HOLD/clock methods; MOCK observer/guards,not native proof."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parents[2]
path = 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeOwner.h'
current = (root/path).read_text()
# Frozen f4908fa428 excerpt;works in independent archives without .git.
old = (here/'pioneer_q38_hold_recheck_before_fix.inc').read_text()
assert hashlib.sha256(old.encode()).hexdigest() == 'f3dddc1a9c8ed5f87037ce46e071564fb788df38080622ebc0ddac9a87910db5'

def extract(source, signature):
    assert source.count(signature) == 1
    begin = source.index(signature)
    brace = source.index('{', begin)
    depth, end = 1, brace+1
    while depth:
        depth += (source[end] == '{')-(source[end] == '}')
        end += 1
    return source[begin:end]

signature = 'template<class IO> bool HoldStopped('
baseline, candidate = extract(old, signature), extract(current, signature)
assert hashlib.sha256(baseline.encode()).hexdigest() == '98585667bf19cbbd8c5a21908efb1f8be658a0d34210e77c1f4bc923cd1b31ca'
clock = extract(current, 'template<class IO> bool BootTime(')
assert clock == extract(old, 'template<class IO> bool BootTime('), 'do not weaken the actual clock/scope guard'
assert baseline != candidate and 'if(!InspectTerminal(io,fresh,true) || !fresh.ready)return Fail(io,r);' in baseline
records = '\n'.join(extract(current, 'struct '+name+' {')+';' for name in ('PioneerQ38TokenTerminalResult', 'PioneerQ38TokenHoldResult'))
assert all(extract(current, 'struct '+name+' {') == extract(old, 'struct '+name+' {') for name in ('PioneerQ38TokenTerminalResult', 'PioneerQ38TokenHoldResult'))
with tempfile.TemporaryDirectory(prefix='pioneer-hold-recheck-method-', dir='/private/tmp') as temporary:
    tmp = pathlib.Path(temporary)
    (tmp/'actual_hold_records.inc').write_text(records+'\n')
    (tmp/'actual_hold_methods.inc').write_text(clock+'\n'+baseline.replace('HoldStopped(', 'OldHoldStopped(', 1)+'\n'+candidate+'\n')
    binary = tmp/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(tmp), str(here/'pioneer_q38_hold_recheck_method_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'UBSAN_OPTIONS':'halt_on_error=1'})
print('ACTUAL_HOLD_METHOD_SHA256 old'+hashlib.sha256(baseline.encode()).hexdigest()+'/new'+hashlib.sha256(candidate.encode()).hexdigest())
print('EXTRACTED METHOD CONTROL FLOW ONLY;observer+guards+clocks are MOCK,NO genuine terminal,model history,native owner,physical or TPS proof')
