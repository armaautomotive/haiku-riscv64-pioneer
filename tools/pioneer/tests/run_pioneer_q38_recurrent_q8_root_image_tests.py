#!/usr/bin/env python3
"""Cross-build and simulate the experimental recurrent controller, no device."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('absolute SDK path required')
sdk=Path(sys.argv[1]);here=Path(__file__).resolve().parent
if not sdk.is_absolute() or not sdk.is_dir():raise SystemExit('SDK path refused')
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku'
with tempfile.TemporaryDirectory(prefix='q38-recurrent-q8-root-',dir='/private/tmp') as directory:
    tmp=Path(directory);obj=tmp/'probe.o';elf=tmp/'probe.elf';text=tmp/'text.bin';binary=tmp/'test'
    subprocess.run([str(tool)+'-g++','-std=c++11','-O2','-ffreestanding','-fno-exceptions','-fno-rtti',
                    '-fno-stack-protector','-fno-pic','-fno-pie','-fno-builtin','-ffunction-sections','-fdata-sections',
                    '-mno-relax','-march=rv32imf','-mabi=ilp32','-fstack-usage','-c',str(here/'pioneer_q38_recurrent_q8_root_probe.cpp'),'-o',str(obj)],check=True)
    subprocess.run([str(tool)+'-ld','-m','elf32lriscv','--no-relax','-Ttext=0x40000','-e','q38_recurrent_q8_root_probe',str(obj),'-o',str(elf)],check=True)
    subprocess.run([str(tool)+'-objcopy','-O','binary','-j','.text',str(elf),str(text)],check=True)
    symbols=subprocess.check_output([str(tool)+'-nm','-n',str(elf)],text=True)
    entries=[line.split()[0] for line in symbols.splitlines() if line.endswith(' T q38_recurrent_q8_root_probe')]
    assert len(entries)==1
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_recurrent_q8_root_image_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(text),entries[0]],check=True,env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
