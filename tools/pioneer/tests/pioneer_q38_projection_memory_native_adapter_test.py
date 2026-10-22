#!/usr/bin/env python3
"""Compile actual kernel memory methods against mock AND real Haiku SDK."""
import os
import pathlib
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
if len(sys.argv) != 2:
    raise SystemExit('expected read-only SDK')
source = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
begin, end = '// PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER_BEGIN', '// PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER_END'
assert source.count(begin) == source.count(end) == 1
snippet = source.split(begin, 1)[1].split(end, 1)[0]
with tempfile.TemporaryDirectory(prefix='pioneer-projection-memory-adapter-') as temporary:
    base = pathlib.Path(temporary)
    include, binary = base / 'actual-native.inc', base / 'test'
    include.write_text(snippet)
    define = '-DPIONEER_PROJECTION_MEMORY_ADAPTER_INCLUDE="' + str(include) + '"'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', define, str(here / 'pioneer_q38_projection_memory_native_adapter_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32', 'UBSAN_OPTIONS': 'halt_on_error=1'})
    sdk = pathlib.Path(sys.argv[1])
    headers = sdk / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
    cross = sdk / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
    system_includes = []
    for path in [headers / 'os', *sorted((headers / 'os').iterdir())]:
        if path.is_dir():
            system_includes += ['-isystem', str(path)]
    subprocess.run([str(cross), '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti',
                    '-ffreestanding', '-fstack-usage', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE', define,
                    '-I' + str(headers), *system_includes, '-idirafter', str(headers / 'posix'), '-c',
                    str(here / 'pioneer_q38_projection_memory_native_kernel_compile.cpp'), '-o', str(base / 'kernel.o')], check=True)
    frames = [int(line.split('\t')[1]) for line in (base / 'kernel.su').read_text().splitlines() if len(line.split('\t')) == 3]
    assert frames and max(frames) <= 2048
    print('MAX_ACTUAL_PROJECTION_MEMORY_NATIVE_KERNEL_FRAME_BYTES=' + str(max(frames)) + ';NO_NATIVE_INSTALL_OR_EXECUTION')
