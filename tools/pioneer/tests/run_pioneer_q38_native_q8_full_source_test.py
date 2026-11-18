#!/usr/bin/env python3
"""Verify the actual Q8 page producer against a hash-authenticated original GGUF."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==2 and Path(sys.argv[1]).is_absolute()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
source=(here.parent/'PioneerQ38PublicModelSource.h').read_text()
first='// PIONEER_Q38_AUTHENTICATED_WEIGHT_PAGE_BEGIN\n'
last='\t// PIONEER_Q38_AUTHENTICATED_WEIGHT_PAGE_END'
assert source.count(first)==source.count(last)==1
body=source.split(first,1)[1].split(last,1)[0]
with tempfile.TemporaryDirectory(prefix='q38-sm7-full-source-') as scratch:
    out=Path(scratch);binary=out/'test'
    (out/'pioneer_q38_native_q8_full_source_page.inc').write_text(body)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-g','-Wall','-Wextra','-Werror',
                    '-Wno-deprecated-declarations','-fsanitize=address,undefined','-DPIONEER_Q38_NATIVE_Q8_FULL_PACKAGE=1',
                    '-I'+scratch,here/'pioneer_q38_native_q8_full_source_test.cpp',
                    here.parent/'PioneerModelWeightCache.cpp','-o',binary],check=True)
    subprocess.run([binary,sys.argv[1]],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
