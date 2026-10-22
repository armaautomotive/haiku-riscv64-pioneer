#!/usr/bin/env python3
"""Standalone embedding ROM is not an old/native role;fail before SDK IO."""
import os
import pathlib
import subprocess
import tempfile

directory = pathlib.Path(__file__).resolve().parents[1]
environment = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-embedding-gates.') as temporary:
    root = pathlib.Path(temporary)
    missing = root / 'MUST_NOT_INSPECT_SDK'
    output = root / 'MUST_NOT_CREATE_OUTPUT'
    for flag in ('PIONEER_Q38_PROJECTION_WORKSPACE', 'PIONEER_Q38_PROJECTION_GRAPH_MEMORY',
                 'PIONEER_Q38_PROJECTION_GRAPH_ROOT', 'PIONEER_Q38_FFN_GRAPH_ROOT',
                 'PIONEER_Q38_GDN_MUTABLE_STATE', 'PIONEER_Q38_EMBEDDING',
                 'PIONEER_UNKNOWN_FUTURE_ROLE'):
        for value in ('1', '2', '-1', '', '00', 'false', '0 ', ' 0', '0=1', '0\n1'):
            result = subprocess.run(['sh', str(directory / 'build_qwen38_embedding_image.sh'),
                                     str(missing), str(output)],
                                    env=dict(environment, **{flag: value}), capture_output=True, text=True)
            assert result.returncode == 2 and not result.stdout and not result.stderr
            assert not output.exists()
            count += 1
    # Explicit0 is not a feature grant;normal SDK validation is reached.
    result = subprocess.run(['sh', str(directory / 'build_qwen38_embedding_image.sh'), str(missing), str(output)],
                            env=dict(environment, PIONEER_Q38_EMBEDDING='0'), capture_output=True, text=True)
    assert result.returncode != 0 and 'MUST_NOT_INSPECT_SDK' in result.stderr and not output.exists()
    count += 1
    # The old local registry cannot build/select the new arithmetic role.
    result = subprocess.run(['sh', str(directory / 'build_qwen38_local_image.sh'), 'embedding',
                             str(missing), str(output)], env=environment, capture_output=True, text=True)
    assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
    count += 1
print(f'PASS embedding standalone build gates{count};inherited nonzero features refused BEFORE SDK/output IO;old role registry unchanged;NO native access')
