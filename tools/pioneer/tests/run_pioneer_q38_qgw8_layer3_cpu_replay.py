#!/usr/bin/env python3
"""Replay one saved dense boundary with CPU-only model arithmetic."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

here = Path(__file__).resolve().parent
workspace = here.parents[2]
converted = workspace / 'generated.pioneer/qualification-inputs/qgw8-hidden-reference.Yn7npdOx/converted-layers-v1.bin'
card = workspace / 'generated.pioneer/qualification-inputs/qgw8-card-trace-20261109/trace.bin'
model = workspace / 'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
for path, digest in ((converted, 'cfe8aaf5e0877a2260c957b2d996ce4626be5779a038886f65934893aef93599'),
                     (card, '103312ce324384782d322b51a457d654c4c5c1fef181ceecc730aa6a3d82af2e')):
    if path.is_symlink() or path.stat().st_size != 64 * 5120 * 4 or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise ValueError('trace identity refused: ' + str(path))
if any(name.startswith(('PIONEER_', 'TT_PIONEER_')) for name in os.environ):
    raise ValueError('role override refused')
source = (here / 'pioneer_q38_cpu_model_reference.cpp').read_text()
start = source.index('struct Q38CpuModelReference::Impl {')
end = source.index('\nQ38CpuModelReference::Q38CpuModelReference()', start)
impl = source[start:end].replace('struct Q38CpuModelReference::Impl {', 'struct Q38LayerReplay {', 1)
with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-layer3-', dir='/private/tmp') as tmp:
    root = Path(tmp)
    (root / 'actual_cpu_replay_impl.inc').write_text(impl)
    executable = root / 'replay'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-I' + str(root),
                    str(here / 'pioneer_q38_qgw8_layer3_cpu_replay.cpp'),
                    str(here / 'pioneer_q38_bfp4_full_cpu_backend.cpp'),
                    str(here / 'pioneer_q38_whole_original_bytes.cpp'),
                    str(here.parent / 'PioneerModelWeightCache.cpp'), '-o', str(executable)], check=True)
    result = subprocess.run([str(executable), str(model), str(converted), str(card)])
    raise SystemExit(result.returncode)
