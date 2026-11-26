#!/usr/bin/env python3
"""Offline same-input check of a captured warm dense-attention continuation.

The supplied tensors must come from separately audited simulator replays with
the same layer input and prior KV. This does not access Pioneer hardware or
provide inference to Work mode.
"""
import argparse
import math
from pathlib import Path
import struct
import subprocess
import tempfile

from pioneer_qwen35_ttsim_service_test import compare_recorded_layer


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('oracle', 'model', 'cpu_trace', 'sim_gated', 'sim_layer', 'sim_residual'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    cpu_files = sorted(p for p in args.cpu_trace.iterdir() if p.is_file())
    if len(cpu_files) < 68:
        raise ValueError('CPU control must contain both positions and their logits and tokens')
    original = (args.cpu_trace / 'trace-attn_gated-3.bin').read_bytes()
    gated = args.sim_gated.read_bytes()
    if len(original) != 4096 * 4 or len(gated) != 4096 * 4:
        raise ValueError('gated tensor must have 4096 float values')
    if any(not math.isfinite(value) for (value,) in struct.iter_unpack('<f', gated)):
        raise ValueError('non-finite simulated attention')

    with tempfile.TemporaryDirectory(prefix='q35-gated-attention-') as temporary:
        root = Path(temporary)
        def run(name, tensor, layer=3):
            directory = root / name
            directory.mkdir()
            completed = subprocess.run([str(args.oracle), str(args.model), str(directory),
                                        'two', '--replace-attn-gated', str(layer),
                                        str(tensor), '--trace-layer', '3'],
                                       capture_output=True, text=True)
            return completed, directory

        control, control_dir = run('control', args.cpu_trace / 'trace-attn_gated-3.bin')
        if control.returncode:
            raise RuntimeError('CPU self-injection failed: ' + control.stderr[-1000:])
        for source in cpu_files:
            if source.read_bytes() != (control_dir / source.name).read_bytes():
                raise AssertionError(f'self-injection changed {source.name}')
        simulated, simulated_dir = run('simulated', args.sim_gated)
        if simulated.returncode:
            raise RuntimeError('simulator injection failed: ' + simulated.stderr[-1000:])
        if (simulated_dir / 'trace-attn_gated-3.bin').read_bytes() != gated:
            raise AssertionError('injected simulator attention was not read back exactly')
        layer_error = compare_recorded_layer(args.sim_layer.read_bytes(),
                                             (simulated_dir / 'position1-layer3.bin').read_bytes())
        residual_error = compare_recorded_layer(args.sim_residual.read_bytes(),
                                                (simulated_dir / 'trace-attn_residual-3.bin').read_bytes())
        short = root / 'short.bin'; short.write_bytes(gated[:-4])
        nonfinite = root / 'nonfinite.bin'; nonfinite.write_bytes(struct.pack('<f', float('nan')) + gated[4:])
        link = root / 'link.bin'; link.symlink_to(args.sim_gated.resolve())
        for name, tensor, layer in (('short', short, 3), ('nonfinite', nonfinite, 3),
                                    ('symlink', link, 3), ('wrong-layer', args.sim_gated, 2)):
            result, _ = run('reject-' + name, tensor, layer)
            if result.returncode == 0:
                raise AssertionError(f'{name} injection was accepted')
    print(f'PASS {len(cpu_files)} control artifacts; gated attention read back exactly; '
          f'layer max abs {layer_error:.9g}, residual max abs {residual_error:.9g}; NO HARDWARE')


if __name__ == '__main__':
    main()
