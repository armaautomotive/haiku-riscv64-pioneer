#!/usr/bin/env python3
"""Separate QFG2 exact frozen data and atomic generator refusal tests."""
import importlib.util
import pathlib
import shutil
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3
here = pathlib.Path(__file__).resolve().parent
script = here.parent / 'generate_qwen38_ffn_input_graph_images.py'
spec = importlib.util.spec_from_file_location('input_graph_generator', script)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
repository = here.parents[2]
positives = refusals = 0

with tempfile.TemporaryDirectory(prefix='pioneer-qfg2-generator-') as temporary:
    root = pathlib.Path(temporary) / 'root'
    peer = pathlib.Path(temporary) / 'peer'
    shutil.copytree(sys.argv[1], root)
    shutil.copytree(sys.argv[2], peer)

    def run(part, refuse=False, optimized=False):
        global positives, refusals
        result = subprocess.run([sys.executable] + (['-O'] if optimized else []) +
                                [str(script), str(root), str(peer), part], capture_output=True, text=True)
        if refuse:
            assert result.returncode and not result.stdout
            refusals += 1
            return
        assert not result.returncode
        suffix = module.PINS[part][3]
        path = 'src/add-ons/kernel/drivers/misc/PioneerQ38FfnInputGraph' + suffix + '.h'
        expected = '*** Begin Patch\n*** Add File: ' + path + '\n'
        expected += ''.join('+' + line + '\n' for line in (repository / path).read_text().splitlines())
        expected += '*** End Patch\n'
        assert result.stdout == expected
        positives += 1

    for part in ['root-main', 'root-prefix', 'root-graph']:
        run(part)
        run(part, optimized=True)
    for part, (section, _, _, _) in module.PINS.items():
        path = (root if part.startswith('root-') else peer) / (section + '.bin')
        saved = path.read_bytes()
        for at in [0, len(saved)//2, len(saved)-1]:
            changed = bytearray(saved)
            changed[at] ^= 1
            path.write_bytes(changed)
            run('root-main', refuse=True)
            run('root-prefix', refuse=True, optimized=True)
        path.write_bytes(saved[:-4])
        run('root-graph', refuse=True)
        path.write_bytes(saved + bytes(4))
        run('root-prefix', refuse=True)
        path.unlink()
        run('root-main', refuse=True)
        path.write_bytes(saved)
    for directory in [root, peer]:
        path = directory / 'boot.bin'
        saved = path.read_bytes()
        for wrong in [b'', bytes(4), saved + bytes(4)]:
            path.write_bytes(wrong)
            run('root-main', refuse=True)
            run('root-graph', refuse=True, optimized=True)
        path.write_bytes(saved)
    for invalid in ['', 'all', '../root-main', 'ROOT-MAIN', 'peer-main', 'peer-affine', 'peer-helper']:
        run(invalid, refuse=True)

print(f'PASS separate QFG2 frozen generator:exact{positives}/refusals{refusals},ALL6sections+both boots BEFORE ANY output,Python-O checked;NEVER_WRITES_OR_REPLACES_QFG1_OR_PEER_NATIVE_CATALOG')
