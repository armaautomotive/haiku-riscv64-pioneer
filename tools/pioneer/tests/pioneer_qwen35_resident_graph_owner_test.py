#!/usr/bin/env python3
"""Exercise the actual driver owner method with the simulated service fleet."""
import os
from pathlib import Path
import subprocess
import tempfile

test_dir = Path(__file__).resolve().parent
root = test_dir.parents[2]
driver = (root / 'src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowDriverBinding.inc').read_text()
begin = '// PIONEER_Q35_RESIDENT_OWNER_INSTALL_BEGIN\n'
end = '// PIONEER_Q35_RESIDENT_OWNER_INSTALL_END'
assert driver.count(begin) == driver.count(end) == 1
method = driver.split(begin, 1)[1].split(end, 1)[0]
with tempfile.TemporaryDirectory() as temporary:
    tmp = Path(temporary)
    (tmp / 'resident-owner-method.inc').write_text(method)
    binary = tmp / 'resident-owner-test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=address,undefined', '-I' + str(tmp),
                    str(test_dir / 'pioneer_qwen35_resident_graph_owner_test.cpp'), '-o', str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([str(binary)], check=True, env=env)
