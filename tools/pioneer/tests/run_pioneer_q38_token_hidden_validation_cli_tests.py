#!/usr/bin/env python3
"""Actual arguments+writer,temporary files only. No model/device execution."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
source=(here.parent/'pioneer_q38_token_hidden_validation.cpp').read_text()
signature='static bool WriteSnapshotExclusive('
assert source.count(signature)==1
begin=source.index(signature);brace=source.index('{',begin);depth=1;end=brace+1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
with tempfile.TemporaryDirectory(prefix='pioneer-hidden75-cli-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary)
    (out/'actual_hidden_snapshot_writer.inc').write_text(source[begin:end]+'\n')
    binary=out/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
        '-fsanitize=address,undefined','-I'+str(out),str(here/'pioneer_q38_token_hidden_validation_cli_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(out)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
