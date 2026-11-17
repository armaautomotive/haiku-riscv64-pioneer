#!/usr/bin/env python3
"""Execute a pinned QGW3 peer through refresh and retained-operand runs."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory


if not __debug__ or len(sys.argv) != 4:
    raise SystemExit('use SDK PINNED_MIXED_ANCESTOR_ELF PINNED_PUSHED_REUSE_ELF')
sdk, old_elf, worker_elf = map(Path, sys.argv[1:])
assert all(p.is_absolute() for p in (sdk,old_elf,worker_elf))
tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
here = Path(__file__).resolve().parent
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',
                   (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with TemporaryDirectory(prefix='q38-raw-ffn-worker-', dir='/private/tmp') as folder:
    root = Path(folder)
    for elf, directory, sha, sections in (
        (old_elf, root/'mixed', '56d5af734ae2d11577239aac470c46fc0300bf5c835eb4a9417974307819ffe6',
         ('boot','mixed','dense','text','affine','ffn','service','gdn')),
        (old_elf.parent/'unchanged-worker/program.elf', root/'mixed/unchanged-worker',
         '31cab4485e0219abbd4c708f106cffd7a57daf53155d6b21afc88351ce1cfe38',
         ('boot','text','affine','ffn','service','gdn')),
        (worker_elf, root/'reuse', '25c286eb0bce198e8c8207c4097eca5f54cce91411dc9ec7f0a484685a72549c',
         ('boot','globalworker','precise','dense','text','affine','ffn','service','gdn'))):
        assert elf.is_file() and not elf.is_symlink()
        data = elf.read_bytes()
        assert hashlib.sha256(data).hexdigest() == sha, elf
        directory.mkdir()
        pin = directory/'program.elf'
        pin.write_bytes(data)
        for section in sections:
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,pin,directory/(section+'.bin')],check=True)
        (directory/'program.symbols').write_bytes(subprocess.check_output([tool+'nm','-n',pin]))
    test = root/'test'
    subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                    *flags,here/'pioneer_q38_raw_ffn_reuse_worker_test.cpp','-o',test],check=True)
    subprocess.run([test,root/'mixed',root/'reuse'],check=True,timeout=120,
                   env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1',ASAN_OPTIONS='quarantine_size_mb=32'))
