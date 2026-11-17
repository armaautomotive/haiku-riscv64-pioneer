#!/usr/bin/env python3
"""Build and execute the matched one-/two-row Q4 images, including vector."""
from hashlib import sha256
import os
from pathlib import Path
import subprocess
import sys

PINS={'one':(3728,'ba94d01ed19833cc3ad998daa84b52f4485f0d065e03312fdcfec7ae7f062e31'),
      'two':(4068,'c11734ef8e5aa76d4d47d7a2bad84b0c4073005a799b57b6affc2a43b57a1226'),
      'single-stream':(4216,'53615bc3cdd0157cde1133795223d8bcf698d2bf57dc324c3119c7256cf11716'),
      'dual-stream':(5576,'fd6c8b885c84bda45093b28b2fc0cb0a3a8934edc60258852e10700666db280b'),
      'dual-vector':(7784,'75555b9c80398a7aa760a2d097512ed983c5125cf9c6a4d97a71c68bd2ccbe5d')}
BOOT='e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'
if not __debug__ or len(sys.argv) not in (3,4) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('use SDK NEW_OUTPUT [MODEL_GGUF] with no build-role environment overrides')
sdk,out=map(Path,sys.argv[1:3])
model=Path(sys.argv[3]).resolve(strict=True) if len(sys.argv)==4 else None
assert model is None or model.is_file()
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=Path(__file__).resolve().parent
for mode,role in (('one','0'),('two','13'),('single-stream','1'),('dual-stream','14'),('dual-vector','15')):
    dest=out/mode
    subprocess.run(['sh',here.parent/'build_qwen38_block_image.sh',sdk,dest],cwd=out,check=True,
                   env=dict(os.environ,PIONEER_Q38_STREAM_IMAGE=role))
    code=(dest/'text.bin').read_bytes()
    assert (len(code),sha256(code).hexdigest())==PINS[mode]
    assert sha256((dest/'boot.bin').read_bytes()).hexdigest()==BOOT
tool=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
source=here.parent/'blackhole_qwen38_two_token_q4.cpp'
marker='-DPIONEER_Q38_TWO_TOKEN_Q4_IMAGE=1'
stream='-DPIONEER_Q38_TWO_TOKEN_Q4_STREAM=1'
vector='-DPIONEER_Q38_TWO_TOKEN_Q4_VECTOR=1'
refusals=((),('-DPIONEER_Q38_TWO_TOKEN_Q4_IMAGE=0',),
    (marker,'-DPIONEER_Q38_TWO_TOKEN_Q4_STREAM=0'),('-DPIONEER_Q38_TWO_TOKEN_Q4_STREAM=1',),
    (marker,vector),(marker,stream,'-DPIONEER_Q38_TWO_TOKEN_Q4_VECTOR=0'),(stream,vector),
    *[(marker,'-D'+m) for m in (
    '_KERNEL_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_WORK_INFERENCE',
    'PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE','__FAST_MATH__')])
for flags in refusals:
    test=subprocess.run([tool,*flags,'-march=rv32imf','-mabi=ilp32','-fsyntax-only',source],
                        cwd=out,text=True,capture_output=True)
    assert test.returncode and any(message in test.stderr for message in (
        'Separate offline two-token Q4 image required','Exact separate two-token Q4 stream role required',
        'Exact separate two-token Q4 vector stream role required')),flags
for flavour,a,b in (('image','one','two'),('stream','single-stream','dual-stream'),
                    ('vector','single-stream','dual-vector')):
    binary=out/('verify-'+flavour)
    subprocess.run([os.getenv('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-unused-function','-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                    here/f'pioneer_q38_two_token_q4_{flavour}_test.cpp','-o',binary],cwd=out,check=True)
    model_args=[model] if flavour=='vector' and model else []
    subprocess.run([binary,out/a,out/b,*model_args],cwd=out,check=True,timeout=120,
                   env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))
print(f'PASS pinned original images unchanged; {len(refusals)} role refusals, no hardware or inference claim')
