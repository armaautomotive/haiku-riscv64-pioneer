#!/usr/bin/env python3
"""Independent rational expected results for actual RV32 BFP4 band images."""
import contextlib
from fractions import Fraction
import io
import os
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile

if not __debug__ or len(sys.argv)!=2:
    raise SystemExit('Use non-optimized Python and the absolute Haiku SDK path')
sdk=Path(sys.argv[1])
assert sdk.is_absolute() and sdk.is_dir()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    rational=runpy.run_path(str(here/'pioneer_native_bfp_arithmetic_test.py'))

def quantized_bf16(n,shift=0):
    # Fixture source is (n/128)*2**shift. Each face-row has max exponent
    # 126+shift and native BFP4 magnitude steps (1/8)*2**shift. Derive round-to-even and
    # saturation from the rational value, not the C++ packer or unpacker.
    q,r=divmod(abs(n),16)
    q=min(7,q+int(r>8 or (r==8 and q%2==1)))
    return rational['encode_normal'](Fraction(-q if n<0 else q,8)*rational['power_of_two'](shift))>>16

assert quantized_bf16(8)==0 and quantized_bf16(24)==0x3e80
assert quantized_bf16(-24)==0xbe80 and quantized_bf16(127)==0x3f60
encoded={(n,shift):quantized_bf16(n,shift) for n in range(-127,128) for shift in range(-6,7)}
with tempfile.TemporaryDirectory(prefix='pioneer-bfp4-rounded-',dir='/private/tmp') as temporary:
    root=Path(temporary)
    test=root/'arithmetic-bfp4'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined',
                    str(here/'pioneer_blackhole_arithmetic_bfp4_band_image_test.cpp'),'-o',str(test)],check=True)
    for width in (5120,6144,17408):
        expected=[]
        for row in range(32):
            total=0
            for tile in range(width//32):
                raw=[((row*37+c*19+tile*11)%255)-127 for c in range(32)]
                assert all(64<=max(abs(x) for x in raw[half:half+16])<=127 for half in (0,16))
                a=[encoded[n,tile%13-6] for n in raw]
                b=[0x3f00+((c*13+tile*7)%256)+(0x8000 if c&3==2 else 0) for c in range(32)]
                for half in (0,16):
                    for phase in (0,2):
                        total=rational['instruction'](a[half:half+16],b[half:half+16],phase,total)
            expected.append(total)
        for mode in ('mmio','direct'):
            output=root/(str(width)+'-'+mode)
            cmd=[sys.executable,str(here.parent/'build_native_bfp4_band.py'),str(sdk),str(output),str(width)]
            if mode=='direct':
                cmd.append('--direct')
            subprocess.run(cmd,check=True)
            subprocess.run([str(test),str(output),str(width),mode],input=' '.join(f'{x:08x}' for x in expected)+'\n',
                           text=True,check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS independent rounded BFP4 predictions for all six actual RV32 image variants; OFFBOARD ONLY')
