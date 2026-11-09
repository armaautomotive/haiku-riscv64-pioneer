#!/usr/bin/env python3
"""Screen captured attention against reference values; no device access."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
workspace=here.parents[2]
hidden=workspace/'generated.pioneer/qualification-inputs/qgw8-hidden-reference.Yn7npdOx/converted-layers-v1.bin'
trace=workspace/'generated.pioneer/qualification-inputs/qgw8-dense-input-physical-20261109/trace.bin'
reference=workspace/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/cpu-attention-input-v1.bin'
model=workspace/'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
for path,size,digest in ((hidden,64*5120*4,'cfe8aaf5e0877a2260c957b2d996ce4626be5779a038886f65934893aef93599'),
                         (trace,64*5120*4,'d26aee7213846431e7bdd223533e6082fd9dc40199174e1228b2a0deefc7024c'),
                         (reference,6144*4,'972b0747f8b03d422a7c5315ccd0fd15bf227231b1c316d5e7026c53eaf0ebf0')):
    if path.is_symlink() or path.stat().st_size!=size or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise SystemExit('pinned trace refused: '+str(path))
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('role override refused')
source=(here/'pioneer_q38_cpu_model_reference.cpp').read_text()
start=source.index('struct Q38CpuModelReference::Impl {')
end=source.index('\nQ38CpuModelReference::Q38CpuModelReference()',start)
implementation=source[start:end].replace('struct Q38CpuModelReference::Impl {','struct Q38LayerReplay {',1)
with tempfile.TemporaryDirectory(prefix='qgw8-value-screen-',dir='/private/tmp') as tmp:
    root=Path(tmp);(root/'actual_cpu_replay_impl.inc').write_text(implementation);binary=root/'screen'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra',
                    '-Werror','-I'+str(root),str(here/'pioneer_q38_dense_value_sign_screen.cpp'),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary),str(model),str(hidden),str(trace),str(reference)],check=True)
