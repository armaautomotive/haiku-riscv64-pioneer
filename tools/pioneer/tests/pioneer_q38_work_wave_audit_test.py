#!/usr/bin/env python3
"""Check real model metadata wave counts; no device or throughput claim."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

assert len(sys.argv)==2
model=Path(sys.argv[1]);assert model.is_absolute() and model.is_file() and not model.is_symlink()
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='work-wave-audit-',dir='/private/tmp') as tmp:
    directory=Path(tmp);binary=directory/'audit';invalid=directory/'invalid'
    invalid.write_bytes(b'not a model')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror',
                    str(here.parent/'pioneer_q38_work_wave_audit.cpp'),'-o',str(binary)],check=True)
    bad=subprocess.run([str(binary),str(invalid)],capture_output=True,text=True)
    assert bad.returncode and not bad.stdout
    good=subprocess.run([str(binary),str(model)],capture_output=True,text=True,check=True)
    assert good.stdout.splitlines()==[
        'MULTIWAVE name=output.weight inputs=5120 outputs=248320 type=14 waves=9',
        'HISTOGRAM waves=1 matrices=496',
        'HISTOGRAM waves=9 matrices=1',
        'TOTAL quantized_matrices=497 multiwave=1 unbatched_calls=505 paired_calls=501 saved_calls=4;STATIC_SHAPES_NOT_RUNTIME_FREQUENCY_OR_SPEED']
    assert not good.stderr
print('PASS actual Qwen metadata/Work scheduler wave-count audit and malformed-file refusal; NO DEVICE')
