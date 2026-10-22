#!/usr/bin/env python3
"""Old builders cannot promote native embedding;fail BEFORE SDK/output."""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

directory = pathlib.Path(__file__).resolve().parents[1]
if len(sys.argv) != 2:
    raise SystemExit('expected exact frozen QPE1 image directory')
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-embedding-native-gates-') as temporary:
    base = pathlib.Path(temporary)
    for name, owner in [('build_qwen38_banks_driver.sh', {}), ('build_qwen38_banks_client.sh', {}), ('build_qwen38_inference.sh', {}),
                        ('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_ROOT': '1'}),
                        ('build_qwen38_ffn_matrix_image.sh', {'PIONEER_Q38_FFN_RESIDENT_MATRIX': '1'}),
                        ('build_qwen38_ffn_persistent_image.sh', {'PIONEER_Q38_FFN_PERSISTENT': '1'}),
                        ('build_qwen38_local_image.sh', {}), ('build_qwen38_projection_graph_image.sh', {}),
                        ('build_qwen38_projection_matrix_image.sh', {}), ('build_qwen38_projection_persistent_image.sh', {})]:
        for value in ('1', '2', '01', '-1', '', '0=1', '0\n1'):
            sdk, output = base / 'MUST_NOT_READ_SDK', base / 'MUST_NOT_CREATE_OUTPUT'
            args = ['sh', str(directory / name), *(['projection_gather'] if name == 'build_qwen38_local_image.sh' else []), str(sdk), str(output)]
            if name == 'build_qwen38_inference.sh':
                args.append(str(base / 'MUST_NOT_READ_LIBRARIES'))
            result = subprocess.run(args, env={**clean, **owner, 'PIONEER_Q38_EMBEDDING_NATIVE': value}, capture_output=True, text=True, timeout=5)
            assert result.returncode == 2 and not output.exists() and str(sdk) not in result.stdout + result.stderr, (name, value, result)
            count += 1
    # Generator pins BOTH immutable sections BEFORE producing ANY patch.
    args = [sys.executable, str(directory / 'generate_qwen38_embedding_image.py')]
    result = subprocess.run([*args, sys.argv[1]], capture_output=True, text=True, check=True)
    lines = result.stdout.splitlines()
    expected = '\n'.join(line[1:] for line in lines if line.startswith('+')) + '\n'
    assert expected == (directory.parents[1] / 'src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingCode.h').read_text()
    image = base / 'image'
    image.mkdir()
    for name in ('code.bin', 'boot.bin'):
        shutil.copyfile(pathlib.Path(sys.argv[1]) / name, image / name)
    for name in ('code.bin', 'boot.bin'):
        original = (image / name).read_bytes()
        for at in (0, len(original) // 2, len(original) - 1):
            data = bytearray(original)
            data[at] ^= 1
            (image / name).write_bytes(data)
            result = subprocess.run([*args, str(image)], capture_output=True, text=True)
            assert result.returncode != 0 and not result.stdout
            count += 1
        (image / name).write_bytes(original)
print(f'PASS native embedding build/catalog gates{count};all old builders refuse BEFORE SDK,complete exact frozen QPE1 code+boot/header pin;NO old-role selector or native install')
