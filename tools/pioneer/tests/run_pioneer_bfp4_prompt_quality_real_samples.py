#!/usr/bin/env python3
"""Verify private real-weight sample identity before/after CPU-only tests."""
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2
folder=Path(sys.argv[1]);assert folder.is_absolute() and not folder.is_symlink()
assert stat.S_IMODE(folder.stat().st_mode)==0o700 and folder.stat().st_uid==os.geteuid()
pins=('edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd',
      '59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695',
      '036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3')
def verify():
 for index,digest in enumerate(pins):
  path=folder/f'band{index}.rbp4';entry=path.lstat()
  assert stat.S_ISREG(entry.st_mode) and entry.st_nlink==1 and entry.st_uid==os.geteuid() and stat.S_IMODE(entry.st_mode)==0o600
  assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
verify()
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-real-quality-',dir='/private/tmp') as temp:
 binary=Path(temp)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off',
                 '-fsanitize=address,undefined',str(here/'pioneer_bfp4_prompt_quality_real_samples_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(folder)],check=True)
verify()
