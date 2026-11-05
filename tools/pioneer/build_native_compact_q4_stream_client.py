#!/usr/bin/env python3
"""Build ONLY the fixed diagnostic104 userspace client;PRIVATE binary,NO stage/device."""
import hashlib
import os
import pathlib
import stat
import subprocess
import sys
assert __debug__ and len(sys.argv) == 4 and not os.environ.get('PYTHONOPTIMIZE')
build, private, output = map(pathlib.Path, sys.argv[1:])
assert build.is_absolute() and build.is_dir() and output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
pins = {'PioneerNativeCompactQ4StreamData.h': '4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
        'PioneerNativeCompactQ4StreamMetadata.h': 'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'}
def unchanged():
    assert private.is_absolute() and not private.is_symlink() and private.stat().st_uid == os.geteuid()
    assert stat.S_IMODE(private.stat().st_mode) == 0o700
    for name, digest in pins.items():
        info = (private / name).lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
        assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
unchanged()
output.mkdir(mode=0o700)
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
cxx = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes = ['-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix')]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
libs = ['-L' + str(path) for path in sorted((build / 'build_packages').glob('gcc_syslibs-*/lib'))]
binary = output / 'pioneer-native-compact-q4-stream-diagnostic'
command = [cxx, '-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror', *includes,
    '-I' + str(private), '-I' + str(here), '-I' + str(root / 'headers/private/shared'), '-I' + str(root / 'headers/private/libroot'),
    '-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot'), '-L' + str(release / 'kits'), *libs,
    str(here / 'pioneer_native_compact_q4_stream_diagnostic.cpp'), str(here / 'PioneerModelWeightCache.cpp'),
    str(release / 'kits/shared/libshared.a'), '-lbe', '-o', str(binary)]
subprocess.run(command, check=True)
(output / 'build-argv.txt').write_text('\t'.join(command) + '\n')
cases = [['-D' + macro + '=' + str(value)] for value in (0, 1) for macro in ('_KERNEL_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE')]
cases.append(['-ffast-math'])
for flags in cases:
    result = subprocess.run([cxx, '-std=c++17', *flags, *includes, '-I' + str(private), '-I' + str(here),
                            '-fsyntax-only', str(here / 'pioneer_native_compact_q4_stream_diagnostic.cpp')], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert result.returncode and 'Compact Q4 full-K source reference is a separate diagnostic' in result.stderr
unchanged()
print(hashlib.sha256(binary.read_bytes()).hexdigest(), binary.name)
print('PASS BUILD_ONLY actual SDK diagnostic104 client LINKED,11 source/Work/validation/fast-math-role refusals;PRIVATE operands unchanged;NO stage/device/hardware/full-model/quality/speed authority')
