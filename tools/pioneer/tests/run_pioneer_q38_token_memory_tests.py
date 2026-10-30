#!/usr/bin/env python3
"""Full-token journal tests with simulated endpoints. NEVER actual device IO."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
args = sys.argv[1:]
precise = '--precise-programs' in args
if precise:
    args.remove('--precise-programs')
sdk = None
if len(args) >= 2 and args[-2] == '--sdk':
    sdk = pathlib.Path(args[-1])
    args = args[:-2]
assert args in ([], ['--positive'], ['--faults'], ['--capsule-faults'])
assert args != ['--capsule-faults'] or precise, 'capsule faults require explicit QTP2 DATA'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-token-memory-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, *(['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1'] if precise else []),
                    str(here/'pioneer_q38_token_memory_test.cpp'), '-o', str(binary)], check=True)
    for extra in ([[]] if args == ['--positive'] else [args] if args in (['--faults'], ['--capsule-faults']) else [[], ['--faults']]):
        subprocess.run([str(binary), *extra], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    if sdk:
        headers = sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
        obj = pathlib.Path(tmp)/'kernel.o'
        tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku')
        subprocess.run([tool+'-g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation',
                        '-fno-exceptions', '-fno-rtti', '-ffreestanding', '-fstack-usage', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE',
                        '-I'+str(headers), '-idirafter', str(headers/'posix'),
                        *(['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1'] if precise else []), '-c',
                        str(here/'pioneer_q38_token_memory_kernel_compile.cpp'), '-o', str(obj)], check=True)
        rows = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        assert rows and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 2048 for row in rows)
        defined = subprocess.check_output([tool+'-nm', '--defined-only', str(obj)], text=True)
        assert all(name in defined for name in ('pioneer_q38_token_memory_template_prepare', 'pioneer_q38_token_memory_template_close'))
        assert not any(line.split()[-1] in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device') for line in defined.splitlines() if line.split())
        unresolved = subprocess.check_output([tool+'-nm', '-u', str(obj)], text=True)
        assert 'TokenMemoryKernelCompileIO' in unresolved and 'PrefixMemoryKernelCompileIO' not in unresolved
        print(f'PASS real RISC-V SDK full-token journal TEMPLATE/maxStaticFrame{max(int(row[1]) for row in rows)}/separate undefined TOKEN IO hooks/no conventional ABI;NOT genuine native context or loadable driver')
