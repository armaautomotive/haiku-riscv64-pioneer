#!/usr/bin/env python3
"""Build and execute the matched single-/two-row Q4 images without a card."""
from hashlib import sha256
import os
from pathlib import Path
import subprocess
import sys

PINS={'one':(3728,'ba94d01ed19833cc3ad998daa84b52f4485f0d065e03312fdcfec7ae7f062e31'),
      'two':(4068,'c11734ef8e5aa76d4d47d7a2bad84b0c4073005a799b57b6affc2a43b57a1226')}
BOOT='e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
if not __debug__ or len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('use SDK NEW_OUTPUT with no build-role environment overrides')
sdk,out=map(Path,sys.argv[1:])
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=Path(__file__).resolve().parent
for mode,role in (('one','0'),('two','13')):
    dest=out/mode
    subprocess.run(['sh',here.parent/'build_qwen38_block_image.sh',sdk,dest],cwd=out,check=True,
                   env=dict(os.environ,PIONEER_Q38_STREAM_IMAGE=role))
    code=(dest/'text.bin').read_bytes()
    assert (len(code),sha256(code).hexdigest())==PINS[mode]
    assert sha256((dest/'boot.bin').read_bytes()).hexdigest()==BOOT
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
source=here.parent/'blackhole_qwen38_two_token_q4.cpp'
marker='-DPIONEER_Q38_TWO_TOKEN_Q4_IMAGE=1'
refusals=((),('-DPIONEER_Q38_TWO_TOKEN_Q4_IMAGE=0',),*[ (marker,'-D'+m) for m in (
    '_KERNEL_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_WORK_INFERENCE',
    'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__')])
for flags in refusals:
    test=subprocess.run([tool,*flags,'-march=rv32imf','-mabi=ilp32','-fsyntax-only',source],
                        cwd=out,text=True,capture_output=True)
    assert test.returncode and 'Separate offline two-token Q4 image required' in test.stderr,flags
binary=out/'verify'
subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                '-Wno-unused-function','-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                here/'pioneer_q38_two_token_q4_image_test.cpp','-o',binary],cwd=out,check=True)
subprocess.run([binary,out/'one',out/'two'],cwd=out,check=True,timeout=120,
               env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
print('PASS pinned original image unchanged; 8 role refusals, no hardware or inference claim')
