#!/usr/bin/env python3
"""Native RISC-V startup DATA compile/stack inspection. NO driver or device IO."""
import hashlib
import os
import pathlib
import subprocess
import sys
assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
sdk, output = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
project = here.parents[2]
assert sdk.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
assert pathlib.Path(tool+'g++').is_file()
output.mkdir(mode=0o700)
obj = output/'token-startup-DATA-NODRV.o'
subprocess.run([tool+'g++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation',
                '-ffreestanding', '-fno-exceptions', '-fno-rtti', '-ffunction-sections', '-fdata-sections', '-fstack-usage',
                *['-I'+str(project/p) for p in ('headers', 'headers/posix', 'headers/config', 'headers/posix/arch/riscv64')],
                '-c', str(here/'pioneer_q38_token_startup_kernel_compile.cpp'), '-o', str(obj)], check=True)
frames, new_frames = [], []
for line in obj.with_suffix('.su').read_text().splitlines():
    name, size, kind = line.split('\t')
    assert kind == 'static' and int(size) <= 2048, (name, size, kind)
    frames.append(int(size))
    if 'PioneerQ38TokenStartupPlan::' in name or 'pioneer_token_startup_' in name:
        assert int(size) <= 512, (name, size, kind)
        new_frames.append(int(size))
assert frames and new_frames
symbols = subprocess.check_output([tool+'nm', '-CS', '--defined-only', str(obj)], text=True)
for name in ('pioneer_token_startup_build', 'pioneer_token_startup_read', 'pioneer_token_startup_destroy',
             'PioneerQ38TokenStartupPlan::Build(', 'PioneerQ38TokenStartupPlan::Matches('):
    assert name in symbols, name
undefined = subprocess.check_output([tool+'nm', '-u', str(obj)], text=True)
for name in ('init_hardware', 'init_driver', 'publish_devices', 'find_device', 'OriginalSource', 'CpuModel',
             'ReferenceProducts', 'ioctl', 'map_physical_memory', 'PioneerQ38RecurrentPrefixOwner'):
    assert name not in symbols+undefined, name
print(f'PASS RISC-V C++11 full startup DATA builder+serializer+page coverage/destructor instantiated/{len(frames)} static frames/newMax{max(new_frames)}B/allMax{max(frames)}B/NO conventional driver ABI/device IO/CPU reference/prefix-owner alias')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' token-startup-DATA-NODRV.o')
print('COMPILE ONLY: not a complete native context,upload authentication,launch/STOP/physical/Work/TPS qualification')
