#!/usr/bin/env python3
"""Check the sharing record format stays in the non-loadable graph witness."""
import os
from pathlib import Path
import subprocess
import tempfile

directory = Path(__file__).resolve().parent
compiler = os.environ.get('CXX', 'c++')
with tempfile.TemporaryDirectory() as temporary:
    source = Path(temporary) / 'guard.cpp'
    def compiles(defines, stream_version, program_version):
        source.write_text('#include "PioneerQwen35DataflowDeviceStream.h"\n'
            f'static_assert(PioneerQwen35::DataflowStreamVersion == {stream_version});\n'
            f'static_assert(PioneerQwen35::DataflowProgramVersion == {program_version});\n')
        return subprocess.run([compiler, '-std=c++17', '-fsyntax-only',
            '-I' + str(directory.parent), *('-D' + x for x in defines), str(source)],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    required = ['PIONEER_QWEN35_DEVICE_DISPATCH', 'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY',
                'PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS', 'PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS']
    assert compiles([], 1, 3)
    assert compiles(required, 2, 4)
    assert compiles([flag for flag in required if flag != 'PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS']
                    + ['PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS'], 2, 4)
    for missing in required[:-1]:
        assert not compiles([flag for flag in required if flag != missing], 2, 4)
    for forbidden in ('PIONEER_QWEN35_RECOVERY_QUALIFY', 'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL'):
        assert not compiles(required + [forbidden], 2, 4)
print('PASS sharing record is versioned and confined to the non-loadable graph witness; OFFLINE ONLY')
