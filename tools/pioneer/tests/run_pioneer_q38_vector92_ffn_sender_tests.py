#!/usr/bin/env python3
"""Execute fixed vector92 baseline and FFN sender ELF bytes in the emulator."""
from hashlib import sha256
import os
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory

if not __debug__ or len(sys.argv)!=3:
    raise SystemExit('use SDK VECTOR92_PAIR')
sdk, pair=map(Path,sys.argv[1:])
assert sdk.is_absolute() and pair.is_absolute()
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
here=Path(__file__).resolve().parent
mixed=pair/'vector92/unchanged-wide/unchanged-twin/unchanged-batched/unchanged-prefetch/unchanged-beta/unchanged-pushed/unchanged-precise/unchanged-global/qualified-math'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
sections=('boot','globalworker','precise','dense','fetch','text','affine','ffn','service','gdn')
with TemporaryDirectory(prefix='q38-vector92-ffn-test-',dir='/private/tmp') as temp:
    root=Path(temp)
    for source,directory,digest,parts in (
            (mixed,root/'mixed','6a74556d6f4095353b0bffd0f44fe3d35706c21ea304f5233d034e47702117ea',
             ('boot','mixed','dense','text','affine','ffn','service','gdn')),
            (mixed/'unchanged-worker',root/'mixed/unchanged-worker','44bac0c1061f298ed8ccd2a1eb4780371bc7c80031cb230ad2da27f476202d19',
             ('boot','text','affine','ffn','service','gdn')),
            (pair/'vector92',root/'vector92','4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870',sections),
            (pair/'reuse',root/'reuse','aefeb0c7cad0eeb1f13867b2c77e473dca4eeb482ad7c9a97cd00d29b19a3f43',sections)):
        original=source/'program.elf'
        assert original.is_file() and not original.is_symlink()
        data=original.read_bytes()
        assert sha256(data).hexdigest()==digest,original
        directory.mkdir()
        elf=directory/'program.elf';elf.write_bytes(data)
        for section in parts:
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,elf,directory/(section+'.bin')],check=True)
        (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',elf]))
    test=root/'test'
    subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                    *flags,here/'pioneer_q38_vector92_ffn_sender_test.cpp','-o',test],check=True)
    subprocess.run([test,root/'mixed',root/'vector92',root/'reuse'],check=True,timeout=120,
                   env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1',ASAN_OPTIONS='quarantine_size_mb=32'))
