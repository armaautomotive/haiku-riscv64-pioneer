#!/usr/bin/env python3
"""Full actual new controller lifetime; only byte endpoints/time are simulated."""
import hashlib
import pathlib
import runpy
import sys
import tempfile
from pioneer_q38_resident_output_execution_common import HERE, compile_test, run_test

assert len(sys.argv) == 8 and sys.argv[7] in [str(i) for i in range(21)]+['--all']
image = pathlib.Path(sys.argv[6]);peer = image/'exact-companion'
for name, (section, size, digest, _) in runpy.run_path(str(HERE.parent/'generate_qwen38_resident_output_images.py'))['PINS'].items():
    data = ((image if name.startswith('root-') else peer)/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
with tempfile.TemporaryDirectory(prefix='pioneer-output-owner-execution-',dir='/private/tmp') as temporary:
    binary = compile_test(pathlib.Path(temporary),['pioneer_q38_resident_output_owner_execution_test','pioneer_q38_resident_output_execution_machine'],sys.argv[1:6])
    run_test(binary,[*sys.argv[1:6],str(image),str(peer),sys.argv[7]])
