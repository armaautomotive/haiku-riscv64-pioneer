#!/usr/bin/env python3
"""Replay normalization/quantization from a pinned layer boundary offline."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent;workspace=here.parents[2]
trace=workspace/'generated.pioneer/qualification-inputs/qgw8-hidden-reference.Yn7npdOx/converted-layers-v1.bin'
model=workspace/'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
if trace.is_symlink() or trace.stat().st_size!=64*5120*4 or hashlib.sha256(trace.read_bytes()).hexdigest()!='cfe8aaf5e0877a2260c957b2d996ce4626be5779a038886f65934893aef93599':
    raise SystemExit('trace identity refused')
source=(here/'pioneer_q38_cpu_model_reference.cpp').read_text()
start=source.index('struct Q38CpuModelReference::Impl {');end=source.index('\nQ38CpuModelReference::Q38CpuModelReference()',start)
impl=source[start:end].replace('struct Q38CpuModelReference::Impl {','struct Q38LayerReplay {',1)
with tempfile.TemporaryDirectory(prefix='qgw8-qkv-input-replay-',dir='/private/tmp') as tmp:
    root=Path(tmp);(root/'actual_cpu_replay_impl.inc').write_text(impl);binary=root/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-I'+str(root),str(here/'pioneer_q38_dense_qkv_input_replay.cpp'),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(model),str(trace)],check=True)
