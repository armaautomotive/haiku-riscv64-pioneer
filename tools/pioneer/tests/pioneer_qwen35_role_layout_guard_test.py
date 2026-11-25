#!/usr/bin/env python3
"""Reject mixing generated role images with the other L1 layout."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if len(sys.argv) != 3:
    raise SystemExit('usage: role_layout_guard_test.py BASELINE_BUILD SHARING_BUILD')
compiler = os.environ.get('CXX', 'c++')
headers = (
    'qwen35-shared-role-images/PioneerQwen35SharedRoleImages.h',
    'qwen35-shared-role-images/PioneerQwen35ExclusiveRoleImages.h',
    'qwen35-local-restore-images/PioneerQwen35LocalRestoreImages.h',
    'qwen35-snapshot-service-restore-images/PioneerQwen35RoleServiceProbeImages.h',
    'qwen35-snapshot-service-restore-images/local-probe/PioneerQwen35LocalRestoreProbeImages.h',
)
with tempfile.TemporaryDirectory() as tmp:
    source = Path(tmp) / 'guard.cpp'
    for directory, sharing in ((Path(sys.argv[1]), False), (Path(sys.argv[2]), True)):
        for relative in headers:
            header = (directory / relative).resolve(strict=True)
            source.write_text(f'#include "{header}"\n')
            for macro in (False, True):
                command = [compiler, '-std=c++17', '-fsyntax-only']
                if macro:
                    command.append('-DPIONEER_QWEN35_ACTIVATION_SHARING_WITNESS')
                result = subprocess.run(command + [str(source)], stdout=subprocess.DEVNULL,
                                        stderr=subprocess.DEVNULL, check=False)
                assert (result.returncode == 0) == (macro == sharing), (header, macro)
print('PASS baseline and sharing role images reject mixed L1 layouts; OFFLINE ONLY')
