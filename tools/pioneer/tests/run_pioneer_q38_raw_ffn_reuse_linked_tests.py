#!/usr/bin/env python3
"""Audit the exact compact V2 or measured-vector92 ROOT pair in RV32."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


vector= len(sys.argv)==5 and sys.argv[-1]=='--vector92'
if not __debug__ or (not vector and len(sys.argv) not in (3,4)):
    raise SystemExit('use SDK PINNED_REUSE_ELF [PINNED_REFETCH_ELF] [--vector92] (without -O)')
sdk, *images = map(Path, sys.argv[1:-1] if vector else sys.argv[1:])
assert sdk.is_absolute()
digests = ('e59cdec3d494e7a264b6904fc6cca62fae49fb82d593ef070c5ff52b383b979d',
           'e5adf9c406d471575a2d5f9b4286db80857a96eb49a1bfff9b94ff706cc43789')
if vector:
    digests=('6258db4e890ec1f1719fad2248ee3994fe6ce4463ce3d938c36d96af125a2010',
             '0fcec876160c9a8b08a584d3f60d8eacee39b0fa96bcac1ce7226b03de59493e')
    # From the pinned images' actual recurrent Matrix -> Run -> MatrixJob
    # call chain, not the second, dense TU's same-named functions.
    sites=('4b398 571e8 4e26c 57174 51a2c 51fe4 4d96c',
           '4b398 57178 4e258 57104 51a18 51fa4 4d958')
tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
here = Path(__file__).resolve().parent
with TemporaryDirectory(prefix='q38-raw-ffn-linked-', dir='/private/tmp') as folder:
    root = Path(folder)
    directories = []
    for elf, digest in zip(images, digests):
        assert elf.is_absolute() and elf.is_file() and not elf.is_symlink()
        image = elf.read_bytes()
        assert hashlib.sha256(image).hexdigest() == digest, elf
        directory = root/str(len(directories))
        directories.append(directory)
        directory.mkdir()
        pinned = directory/'checked.elf'
        pinned.write_bytes(image)
        for section in ('text','conv'):
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,pinned,directory/(section+'.bin')],check=True)
        if vector:
            (directory/'sites.txt').write_text(sites[len(directories)-1]+'\n')
    test = root/'test'
    subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-misleading-indentation','-fsanitize=address,undefined',
                    here/'pioneer_q38_raw_ffn_reuse_linked_test.cpp','-o',test],check=True)
    subprocess.run([test,*directories,*(['--vector92'] if vector else [])],check=True,timeout=60,
                   env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
