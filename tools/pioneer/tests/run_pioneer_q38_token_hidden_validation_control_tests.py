#!/usr/bin/env python3
"""Actual frontend,MOCK lifecycle/reader/user-copy endpoints. No device access."""
import os
import pathlib
import re
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
role=(kernel/'PioneerQ38TokenHiddenValidationRole.h').read_text()
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
assert len(required)==23
owner=(kernel/'PioneerQ38TokenNativeOwner.h').read_text()
begin=owner.index('struct PioneerQ38TokenHiddenReadResult {')
end=owner.index('\n};',begin)+3
with tempfile.TemporaryDirectory(prefix='pioneer-token-hidden-control-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary)
    (out/'hidden_validation_role_defines.inc').write_text(''.join('#define '+name+' 1\n' for name in sorted(required))+
        '#define PIONEER_DRIVER_BUILD_TAG 0x2026103100010075ULL\n')
    (out/'actual_hidden_result.inc').write_text(owner[begin:end]+'\n')
    binary=out/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
        '-fsanitize=address,undefined','-I'+str(out),'-I'+str(kernel),
        str(here/'pioneer_q38_token_hidden_validation_control_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
