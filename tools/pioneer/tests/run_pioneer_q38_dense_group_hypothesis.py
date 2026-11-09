#!/usr/bin/env python3
"""Screen first-token dense head group hypotheses using CPU-only arithmetic."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
workspace = here.parents[2]
layers = workspace / 'generated.pioneer/qualification-inputs/qgw8-hidden-reference.Yn7npdOx/converted-layers-v1.bin'
trace = workspace / 'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/trace.bin'
cpu = workspace / 'generated.pioneer/qualification-inputs/qgw8-card-trace-20261109/cpu-dense-stage-v1.bin'
model = workspace / 'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
for path, size, expected in ((layers, 64 * 5120 * 4, 'cfe8aaf5e0877a2260c957b2d996ce4626be5779a038886f65934893aef93599'),
                             (trace, 64 * 5120 * 4, 'cf9448bb92349b5e38409ee02e6e80abc9824f2cf2454f18d8633fd5ff1f9ae1'),
                             (cpu, 2 * 5120 * 4, 'cb6544d071ce526de7ec7cab66189b2ca39f73ecb33111dbd46b0bfeed2c5f8d')):
    if path.is_symlink() or path.stat().st_size != size or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise ValueError('trace identity refused: ' + str(path))
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise ValueError('role override refused')
source = (here / 'pioneer_q38_cpu_model_reference.cpp').read_text()
start = source.index('struct Q38CpuModelReference::Impl {')
end = source.index('\nQ38CpuModelReference::Q38CpuModelReference()', start)
implementation = source[start:end].replace('struct Q38CpuModelReference::Impl {', 'struct Q38LayerReplay {', 1)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-groups-', dir='/private/tmp') as tmp:
    root = Path(tmp)
    (root / 'actual_cpu_replay_impl.inc').write_text(implementation)
    binary = root / 'screen'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-I' + str(root),
                    str(here / 'pioneer_q38_dense_group_hypothesis.cpp'),
                    str(here / 'pioneer_q38_whole_original_bytes.cpp'),
                    str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(model), str(layers), str(trace), str(cpu)], check=True)
