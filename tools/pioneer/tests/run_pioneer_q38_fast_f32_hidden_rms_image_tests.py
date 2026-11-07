#!/usr/bin/env python3
"""Compare real RV32 RMS images on synthetic simulated endpoints only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv) in (2,4,6) and (len(sys.argv)==2 or sys.argv[2]=='--prompt-fixture')
assert len(sys.argv)!=6 or sys.argv[4]=='--ffn-fixture'
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk=Path(sys.argv[1]);assert sdk.is_absolute() and sdk.is_dir()
fixture=Path(sys.argv[3]) if len(sys.argv)>=4 else None
ffn=Path(sys.argv[5]) if len(sys.argv)==6 else None
assert fixture is None or (fixture.is_absolute() and fixture.is_file())
assert ffn is None or (ffn.is_absolute() and ffn.is_file())
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-fast-rms-image-',dir='/private/tmp') as temporary:
    temp=Path(temporary);images=temp/'images';binary=temp/'test'
    subprocess.run([sys.executable,str(here.parent/'build_qwen38_fast_f32_hidden_rms_test_image.py'),str(sdk),str(images)],check=True)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_q38_fast_f32_hidden_rms_image_test.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary),str(images/'original'),str(images/'candidate')],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
    if fixture is not None:
        prompt=temp/'prompt-test'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-ffp-contract=off','-fsanitize=address,undefined',str(here/'pioneer_q38_fast_rms_prompt_image_test.cpp'),
                        str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(prompt)],check=True)
        subprocess.run([str(prompt),str(images/'original'),str(images/'candidate'),str(fixture),
                        *([str(ffn)] if ffn else [])],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
        with fixture.open('rb') as source:
            payload=bytearray(source.read(20881))
        assert len(payload)==20880
        payload[16]^=1;corrupt=temp/'corrupt-prompt.bin';corrupt.write_bytes(payload)
        refused=subprocess.run([str(prompt),str(images/'original'),str(images/'candidate'),str(corrupt)],
                               capture_output=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32',
                                                        'UBSAN_OPTIONS':'halt_on_error=1'})
        assert refused.returncode==2 and not refused.stdout
        if ffn:
            with ffn.open('rb') as source:
                altered=bytearray(source.read(290849))
            assert len(altered)==290848
            altered[32]^=1;bad_ffn=temp/'corrupt-ffn.bin';bad_ffn.write_bytes(altered)
            refused=subprocess.run([str(prompt),str(images/'original'),str(images/'candidate'),str(fixture),str(bad_ffn)],
                                   capture_output=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32',
                                                            'UBSAN_OPTIONS':'halt_on_error=1'})
            assert refused.returncode==2 and not refused.stdout
        print('PASS changed private Qwen fixture refused before instruction simulation',flush=True)
