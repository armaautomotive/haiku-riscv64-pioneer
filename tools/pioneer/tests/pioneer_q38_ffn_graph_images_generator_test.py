#!/usr/bin/env python3
"""Exact frozen-data regeneration and refusal before any patch output."""
import importlib.util
import pathlib
import shutil
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
script = here.parent / 'generate_qwen38_ffn_graph_images.py'
spec = importlib.util.spec_from_file_location('graph_image_generator', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
repository = here.parents[2]
positives = 0
refusals = 0

with tempfile.TemporaryDirectory(prefix='pioneer-ffn-graph-image-generator-') as temporary:
    root = pathlib.Path(temporary) / 'root'
    peer = pathlib.Path(temporary) / 'peer'
    shutil.copytree(sys.argv[1], root)
    shutil.copytree(sys.argv[2], peer)

    def run(part, refuse=False, optimized=False):
        global positives, refusals
        command = [sys.executable] + (['-O'] if optimized else []) + [str(script), str(root), str(peer), part]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if refuse:
            assert result.returncode != 0 and not result.stdout
            refusals += 1
        else:
            assert result.returncode == 0
            suffix = module.PINS[part][3]
            path = 'src/add-ons/kernel/drivers/misc/PioneerQ38FfnGraph' + suffix + '.h'
            expected = '*** Begin Patch\n*** Add File: ' + path + '\n'
            expected += ''.join('+' + line + '\n' for line in (repository / path).read_text().splitlines())
            expected += '*** End Patch\n'
            assert result.stdout == expected
            positives += 1

    for part in module.PINS:
        run(part)
        run(part, optimized=True)
    # Every section is checked even if a DIFFERENT section was requested.
    for part, (section, _, _, _) in module.PINS.items():
        path = (root if part.startswith('root-') else peer) / (section + '.bin')
        saved = path.read_bytes()
        for position in [0, len(saved)-1]:
            changed = bytearray(saved)
            changed[position] ^= 1
            path.write_bytes(changed)
            run('peer-helper' if part != 'peer-helper' else 'root-main', refuse=True)
            run('root-main', refuse=True, optimized=True)
        path.write_bytes(saved[:-4])
        run('root-main', refuse=True)
        path.write_bytes(saved + bytes(4))
        run('peer-main', refuse=True)
        path.unlink()
        run('root-prefix', refuse=True)
        path.write_bytes(saved)
    for directory in [root, peer]:
        path = directory / 'boot.bin'
        saved = path.read_bytes()
        for wrong in [b'', b'\x00'*4, saved + b'\x00'*4]:
            path.write_bytes(wrong)
            run('root-main', refuse=True)
        path.write_bytes(saved)
    for invalid in ['', 'all', '../root-main', 'ROOT-MAIN']:
        run(invalid, refuse=True)

print(f'PASS private frozen image generator:exact regeneration{positives}/refusals{refusals};ALL6sections+both boots pinned BEFORE output,Python-O does not bypass checks;NO_NATIVE_ACCESS')
