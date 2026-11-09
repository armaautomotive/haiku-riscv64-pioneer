#!/usr/bin/env python3
"""Sample original dense Q8 -> BFP8 fidelity; CPU-only, no cache/device."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
here=Path(__file__).resolve().parent;workspace=here.parents[2]
hidden=workspace/'generated.pioneer/qualification-inputs/qgw8-hidden-reference.Yn7npdOx/converted-layers-v1.bin'
model=workspace/'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
if hidden.is_symlink() or hidden.stat().st_size!=64*5120*4 or hashlib.sha256(hidden.read_bytes()).hexdigest()!='cfe8aaf5e0877a2260c957b2d996ce4626be5779a038886f65934893aef93599':
    raise SystemExit('hidden input identity refused')
source=(here/'pioneer_q38_cpu_model_reference.cpp').read_text()
start=source.index('struct Q38CpuModelReference::Impl {');end=source.index('\nQ38CpuModelReference::Q38CpuModelReference()',start)
implementation=source[start:end].replace('struct Q38CpuModelReference::Impl {','struct Q38LayerReplay {',1)
with tempfile.TemporaryDirectory(prefix='q38-dense-q8-bfp8-sample-',dir='/private/tmp') as tmp:
    tmp=Path(tmp);(tmp/'actual_cpu_replay_impl.inc').write_text(implementation);binary=tmp/'sample'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra',
                    '-Werror','-I'+str(tmp),str(here/'pioneer_q38_dense_q8_bfp8_sample.cpp'),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),
                    '-o',str(binary)],check=True)
    for role in ('_KERNEL_MODE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT'):
        result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-I'+str(tmp),'-D'+role+'=1',
                               '-fsyntax-only',str(here/'pioneer_q38_dense_q8_bfp8_sample.cpp')],capture_output=True)
        assert result.returncode and b'Offline numerical sample only' in result.stderr,role
    subprocess.run([str(binary),str(model),str(hidden)],check=True)
