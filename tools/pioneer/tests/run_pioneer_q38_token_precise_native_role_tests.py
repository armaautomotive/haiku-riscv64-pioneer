#!/usr/bin/env python3
"""QTP2 native compile-profile separation; no link, runtime or device access."""
import os
import pathlib
import re
import subprocess

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = kernel/'PioneerQ38TokenNativeRole.h'
text = role.read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', text))
forbidden = set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)', text[text.index('#if defined(PIONEER_Q38_EMBEDDING_NATIVE)'):]))
assert len(parents) == 21 and len(forbidden) == 33 and not parents & forbidden
profile = 'PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT'
program = 'PIONEER_Q38_TOKEN_PRECISE_PROGRAMS'
tag = 'PIONEER_DRIVER_BUILD_TAG'
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
base = {**dict.fromkeys(parents, '1'), profile:'1', program:'1', tag:'0x5154324e4f445256ULL',
        **{name:'pioneer_q38_token_context_body_'+name for name in abi}}
include = '#include "'+str(role)+'"\n'
accepted = refused = 0


def check(definitions, accept, tail='', body=include, diagnostic=''):
    global accepted, refused
    source = ''.join('#define '+key+' '+value+'\n' for key, value in sorted(definitions.items()))+body+tail
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-I'+str(kernel),
                             '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True,
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    assert (result.returncode == 0) == accept and diagnostic in result.stderr, (accept, result.stderr)
    if accept:
        accepted += 1
    else:
        assert 'error:' in result.stderr
        refused += 1


check(base, True, include)  # repeated exact profile is stable
for name in sorted(parents | {profile, program}):
    missing = dict(base)
    del missing[name]
    check(missing, False)
    check(base, False, '#undef '+name+'\n'+include)
    for value in ('0', '2'):
        check({**base, name:value}, False)
        check(base, False, '#undef '+name+'\n#define '+name+' '+value+'\n'+include)
for name in abi:
    missing = dict(base)
    del missing[name]
    check(missing, False)
for value in ('0', '0x51544b4e4f445256ULL', '0x5152504e4f445256ULL', '0x2026102700010072ULL'):
    check({**base, tag:value}, False)
    check(base, False, '#undef '+tag+'\n#define '+tag+' '+value+'\n'+include)
for name in sorted(forbidden):
    for value in ('0', '1'):
        check({**base, name:value}, False)
        check(base, False, '#define '+name+' '+value+'\n'+include)

# Atomic valid-profile replacement must still fail: every leaf stays bound
# to its original family,not just individually valid later macro values.
old = dict(base)
del old[profile]
del old[program]
old[tag] = '0x51544b4e4f445256ULL'
switch_new = '#undef '+tag+'\n#define '+tag+' '+base[tag]+'\n#define '+profile+' 1\n#define '+program+' 1\n'
switch_old = '#undef '+profile+'\n#undef '+program+'\n#undef '+tag+'\n#define '+tag+' '+old[tag]+'\n'
check(old, False, switch_new+include, diagnostic='late QTP2 native profile change')
check(base, False, switch_old+include, diagnostic='late QTP1 native profile change')

# Compile actual immutable program/journal/boot-family DATA in the exact new
# native profile;the flag alone must never manufacture native ownership.
data = '#include "'+str(here.parent/'PioneerQ38TokenPrograms.h')+'"\n'
data += '#include "'+str(here.parent/'PioneerQ38TokenFootprint.h')+'"\n'
data += '#include "'+str(here.parent/'PioneerQ38SelectedGlobalWorker.h')+'"\n'
check(base, True, 'static_assert(PioneerQ38TokenNativeProfile==2,"profile");\n'
      'static_assert(PioneerQ38TokenPrograms::Family==0x32505451,"program");\n'
      'static_assert(PioneerQ38TokenL1::SavedBytes==53426408,"journal");\n'
      'static_assert(PioneerQ38SelectedGlobalWorker::Magic==0x32574751,"worker");\n', body=data)
check({profile:'1', program:'1'}, False, body=data)
check({profile:'1'}, False, body=include+data)

# Real driver's first tag/witness block,not a handwritten substitute.
driver = (kernel/'tenstorrent_pci_probe.cpp').read_text()
marker = '#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_ROLE_TAG_OK'
assert driver.count(marker) == 1
prefix = driver.split(marker, 1)[0]
check(base, True, body=prefix)
for value in ('0', '1'):
    check({**base, 'PIONEER_Q38_TOKEN_NATIVE_ROLE_TAG_OK':value}, False, body=prefix,
          diagnostic='Internal token NODRV witness cannot be supplied')
missing = dict(base)
del missing['PIONEER_Q38_TOKEN_NATIVE_CONTEXT']
check(missing, False, body=prefix, diagnostic='Reserved QT2NODRV tag requires its exact')
print(f'PASS distinct QT2NODRV/QTP2 native profile/{accepted} positive/{refused} missing+zero+two+late+wrong-family+CPU/Work refusals/21 parents+6 ABI aliases/actual driver prefix+53MB journal/QGW2 boot binding;NO link/install/runtime/physical/Work grant')
