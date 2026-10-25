#!/usr/bin/env python3
"""Fixed DEBUG geometry + actual read-only Haiku SDK compile; NO card IO."""
import os
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit(2)
here = pathlib.Path(__file__).resolve().parent
sdk = pathlib.Path(sys.argv[1])
develop = sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
cross = sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
with tempfile.TemporaryDirectory(prefix='pioneer-output-readback-plan-', dir='/private/tmp') as temporary:
    directory = pathlib.Path(temporary)
    binary = directory/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(here/'pioneer_q38_resident_output_readback_plan_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32',
                                                 'UBSAN_OPTIONS':'halt_on_error=1'})
    subprocess.run([str(cross), '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti',
                    '-ffreestanding', '-fstack-usage', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE',
                    '-DPIONEER_Q38_GDN_MUTABLE_STATE', '-I'+str(develop/'headers'), '-idirafter',
                    str(develop/'headers/posix'), '-c',
                    str(here/'pioneer_q38_resident_output_readback_plan_kernel_compile.cpp'),
                    '-o', str(directory/'kernel.o')], check=True)
    frames = []
    for line in (directory/'kernel.su').read_text().splitlines():
        fields = line.split('\t')
        if len(fields) == 3:
            assert fields[2] == 'static' and int(fields[1]) <= 2048, line
            frames.append(int(fields[1]))
    assert frames
    print('PASS actual Haiku RV64 fixed resident-output readback DATA compile/MAX_KERNEL_FRAME_BYTES='
          +str(max(frames))+';SDK read-only,NO driver link/install/native read permission')
