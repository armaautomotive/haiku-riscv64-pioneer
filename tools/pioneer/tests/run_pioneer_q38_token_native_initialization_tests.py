#!/usr/bin/env python3
"""Full-model native initializers with actual frozen zero RV32; NO device IO."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) in (2,3) and (len(sys.argv) == 2 or sys.argv[2] in ('--faults', '--late-faults', '--held', '--held-faults', '--runtime', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults'))
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image = pathlib.Path(sys.argv[1])
assert image.is_absolute() and hashlib.sha256((image/'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
defines = dict.fromkeys(parents, '1')
defines['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
defines.update({name:'pioneer_token_init_test_'+name for name in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')})
defines.update(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE='1', TT_PIONEER_EXPERIMENTAL_Q38_BLOCK='1')
if len(sys.argv) == 3 and sys.argv[2] in ('--held', '--held-faults', '--runtime', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE'] = '1'
if len(sys.argv) == 3 and sys.argv[2] in ('--runtime', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE'] = '1'
if len(sys.argv) == 3 and sys.argv[2] in ('--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE'] = '1'
if len(sys.argv) == 3 and sys.argv[2] in ('--execution', '--execution-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE'] = '1'
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-token-native-init-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    session = source[source.index('struct PioneerSessionIO :'):]
    signature = '\tvoid QuarantineHostPage() {'
    assert session.count(signature) == 1
    begin = session.index(signature)
    at = session.index('{', begin)
    depth, end = 1, at+1
    while depth:
        depth += (session[end] == '{')-(session[end] == '}')
        end += 1
    (base/'token_native_host_quarantine.inc').write_text(session[begin:end]+'\n')
    start, end = '\tbool Unsafe(\n', '\n\tvoid Snapshot(tt_resident_session_result& r)'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_unsafe.inc').write_text(start+source.split(start, 1)[1].split(end, 1)[0])
    for name, marker in (('lock', 'PIONEER_PROBE_LOCK'), ('retain', 'PIONEER_Q38_RETAIN_MEMORY')):
        start, end = '// '+marker+'_BEGIN', '// '+marker+'_END'
        assert source.count(start) == source.count(end) == 1
        (base/('token_native_'+name+'.inc')).write_text(source.split(start, 1)[1].split(end, 1)[0])
    start = 'EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)\n{\n'
    assert source.count(start) == 1
    branch = source.split(start, 1)[1].split('\n#endif', 1)[0]+'\n#endif\n'
    assert branch.startswith('#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT\n')
    (base/'token_native_end.inc').write_text(branch)

    def method(signature):
        assert source.count(signature) == 1
        begin = source.index(signature)
        at = source.index('{', begin)
        depth, end = 1, at+1
        while depth:
            depth += (source[end] == '{')-(source[end] == '}')
            end += 1
        return source[begin:end]+'\n'

    (base/'token_native_zero_callbacks.inc').write_text(''.join(method(signature) for signature in
        ('bool PioneerSessionIO::StateZeroBeforeCohort()', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()', 'bool PioneerSessionIO::NonRootHeldOwner()')))
    binary = base/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    '-I'+str(base), '-I'+str(kernel), str(here/'pioneer_q38_token_native_initialization_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:]], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
