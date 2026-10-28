#!/usr/bin/env python3
"""Pure observer bookkeeping proof, no model/device instructions."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
with tempfile.TemporaryDirectory(prefix='pioneer-instruction-windows-',dir='/private/tmp') as tmp:
 binary=pathlib.Path(tmp)/'test'
 profile=pathlib.Path(tmp)/'profile.map'
 profile.write_text('40000 8 first\n40008 4 second\n5fffc 4 last-root\n64000 4 first-conv\n6bffc 4 last-conv\n6000 4 boot\n60000 8 terminal\n')
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  str(here/'pioneer_q38_instruction_windows_test.cpp'),'-o',str(binary)],check=True)
 result=subprocess.check_output([str(binary),str(profile)],text=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 assert result.splitlines()[1:]==[
  'ROOT_INSTRUCTION_PROFILE total10/functions7/EXCLUSIVE actual PCs incl SIMULATED polling/NOT physical cycles or token speed',
  'ROOT_INSTRUCTION_FUNCTION 3 first','ROOT_INSTRUCTION_FUNCTION 2 terminal',
  *('ROOT_INSTRUCTION_FUNCTION 1 '+name for name in ('second','last-root','first-conv','last-conv','boot'))],result
 print(result,end='')
