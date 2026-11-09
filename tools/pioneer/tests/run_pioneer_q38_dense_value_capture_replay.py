#!/usr/bin/env python3
"""Hash-check value capture and replay against the CPU model offline."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile
here=Path(__file__).resolve().parent;workspace=here.parents[2]
hidden=workspace/'generated.pioneer/qualification-inputs/qgw8-hidden-reference.Yn7npdOx/converted-layers-v1.bin'
reference=workspace/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/cpu-attention-input-v1.bin'
model=workspace/'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
p=argparse.ArgumentParser(description=__doc__);p.add_argument('capture',type=Path);p.add_argument('--sha256',required=True);args=p.parse_args()
if not re.fullmatch('[0-9a-f]{64}',args.sha256) or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('digest or role override refused')
for path,size,digest in ((hidden,64*5120*4,'cfe8aaf5e0877a2260c957b2d996ce4626be5779a038886f65934893aef93599'),
                         (reference,6144*4,'972b0747f8b03d422a7c5315ccd0fd15bf227231b1c316d5e7026c53eaf0ebf0'),
                         (args.capture,64*5120*4,args.sha256)):
    if not path.is_absolute() or path.is_symlink() or path.stat().st_size!=size or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise SystemExit('data identity refused: '+str(path))
source=(here/'pioneer_q38_cpu_model_reference.cpp').read_text()
start=source.index('struct Q38CpuModelReference::Impl {');end=source.index('\nQ38CpuModelReference::Q38CpuModelReference()',start)
implementation=source[start:end].replace('struct Q38CpuModelReference::Impl {','struct Q38LayerReplay {',1)
with tempfile.TemporaryDirectory(prefix='qgw8-value-replay-',dir='/private/tmp') as tmp:
    tmp=Path(tmp);(tmp/'actual_cpu_replay_impl.inc').write_text(implementation);binary=tmp/'replay'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra',
                    '-Werror','-I'+str(tmp),str(here/'pioneer_q38_dense_value_capture_replay.cpp'),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary),str(model),str(hidden),str(args.capture),str(reference)],check=True)
