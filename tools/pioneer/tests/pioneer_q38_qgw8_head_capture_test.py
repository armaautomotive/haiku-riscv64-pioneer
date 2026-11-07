#!/usr/bin/env python3
"""Exercise only capture safety, using inert /usr/bin/true and false."""
from pathlib import Path
import subprocess
import sys
import tempfile

for name in ('capture_pioneer_q38_qgw8_head_compare.py', 'capture_pioneer_q38_mask11_compare.py'):
    wrapper = Path(__file__).with_name(name)
    with tempfile.TemporaryDirectory(prefix='pioneer-qgw8-head-capture-') as directory:
        root = Path(directory)
        model = root / 'no-model'
        success, failure = root / 'success', root / 'failure'
        def run(binary, log):
            return subprocess.run([sys.executable, str(wrapper), binary, str(model), str(log)],
                                  text=True, capture_output=True)
        good = run('/usr/bin/true', success)
        assert good.returncode == 0 and success.read_text() == good.stdout
        assert success.read_text().endswith('CAPTURE_EXIT 0\n') and success.stat().st_mode & 0o777 == 0o600
        before = success.read_bytes()
        assert run('/usr/bin/true', success).returncode and success.read_bytes() == before
        bad = run('/usr/bin/false', failure)
        assert bad.returncode and failure.read_text() == bad.stdout and bad.stdout.endswith('CAPTURE_EXIT 1\n')
        invalid = subprocess.run([sys.executable, str(wrapper), 'relative', str(model), str(root / 'unused')], capture_output=True)
        assert invalid.returncode and not (root / 'unused').exists()
print('PASS both private captures, no overwrite, argument and child-failure checks; no model/device')
