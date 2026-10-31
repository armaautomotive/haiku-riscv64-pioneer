#!/usr/bin/env python3
"""Build/inspect DEBUG74; no install/run. Source-only runtime is a separate step.

The native three-position mode still requires independent deployment gates.
"""
import hashlib
import os
import pathlib
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit(2)
build, out = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
project = here.parents[1]
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
sdk = build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build/'objects/haiku/riscv64/release'
tool = build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-'
cxx = str(tool)+'g++'
includes = ['-I'+str(sdk/'headers'), '-idirafter', str(sdk/'headers/posix')]
for path in [sdk/'headers/os', *sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
libraries = ['-L'+str(path) for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
flags = ['-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti',
         '-fstack-usage', '-ffile-prefix-map='+str(project)+'/=/pioneer-source/']
obj = out/'client.o'
subprocess.run([cxx, *flags, '-DPIONEER_Q38_TOKEN_VALIDATION_CLIENT=1', *includes,
                '-c', str(here/'pioneer_q38_token_validation.cpp'), '-o', str(obj)], check=True)
binary = out/'pioneer-q38-token-DEBUG74-DIAGNOSTIC-ONLY'
subprocess.run([cxx, *includes, *libraries, '-B'+str(release/'system/glue')+'/',
                '-L'+str(release/'system/libroot'), str(obj), '-o', str(binary)], check=True)
symbols = subprocess.check_output([str(tool)+'nm', '-C', str(binary)], text=True)
for required in ('PioneerQ38TokenValidationSession', 'PioneerQ38PublicModelSource', 'ioctl', 'sha256sum',
                 'PioneerQwen38PackedPlan'):
    if required == 'sha256sum':
        assert b'/bin/sha256sum\0' in binary.read_bytes()
    else:
        assert required in symbols, required
for forbidden in ('llama_', 'ggml_', 'ReferenceProducts', 'OriginalSource', 'PioneerSessionIO', 'PioneerStoreContext',
                  'Q38WorkControl', 'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'SyntheticIO', 'SyntheticSource'):
    assert forbidden not in symbols, forbidden
rows = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 32768 for row in rows), rows
assert b'--validate-three-positions\0' in binary.read_bytes()
assert b'--preflight-source-only\0' in binary.read_bytes()
assert b'--validate-cancel-boundary\0' in binary.read_bytes()
assert b'--host-timing\0' in binary.read_bytes()
for path in (obj, binary):
    print(hashlib.sha256(path.read_bytes()).hexdigest()+' '+path.name)
print('PASS actual Haiku riscv64 DEBUG74 USER client compile/link/fixed same-open full SHA before device Open/ALL learned parts/no CPU oracle+Work+mock/native imports/maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows)))
print('BUILD+INSPECT ONLY;explicit source-only checks grant NO native execution;three-position runtime still requires independent native/lifecycle/stack/numerical qualification;NO physical or TPS proof')
