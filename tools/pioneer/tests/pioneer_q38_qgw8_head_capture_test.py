#!/usr/bin/env python3
"""Exercise only capture safety, using inert /usr/bin/true and false."""
from pathlib import Path
import subprocess
import sys
import tempfile

for name in ('capture_pioneer_q38_qgw8_head_compare.py', 'capture_pioneer_q38_mask11_compare.py',
             'capture_pioneer_q38_window11_early_compare.py'):
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
        if name == 'capture_pioneer_q38_qgw8_head_compare.py':
            optional = root / 'optional'
            extra = subprocess.run([sys.executable, str(wrapper), '/usr/bin/true', str(model), str(optional), '108133'],
                                   text=True, capture_output=True)
            assert extra.returncode == 0 and optional.read_text() == extra.stdout and 'observed=108133' in extra.stdout
            saved = root / 'saved-capture'
            extra = subprocess.run([sys.executable, str(wrapper), '/usr/bin/true', str(model), str(saved), '108133',
                                    '--save-hidden', str(root / 'snapshot')], text=True, capture_output=True)
            assert extra.returncode == 0 and 'hidden_snapshot=requested' in extra.stdout
            invalid_path = root / 'invalid-hidden'
            bad = subprocess.run([sys.executable, str(wrapper), '/usr/bin/true', str(model), str(invalid_path), '108133',
                                  '--save-hidden', 'relative'], capture_output=True)
            assert bad.returncode and not invalid_path.exists()
            existing = root / 'existing-snapshot'
            existing.write_text('preserve')
            blocked = root / 'blocked-capture'
            bad = subprocess.run([sys.executable, str(wrapper), '/usr/bin/true', str(model), str(blocked), '108133',
                                  '--save-hidden', str(existing)], capture_output=True)
            assert bad.returncode and not blocked.exists() and existing.read_text() == 'preserve'
            for token in ('-1', '248320', '1234567', '1x', '１２'):
                rejected = root / ('rejected-' + str(len(token)))
                result = subprocess.run([sys.executable, str(wrapper), '/usr/bin/true', str(model), str(rejected), token],
                                        capture_output=True)
                assert result.returncode and not rejected.exists()
print('PASS all three private captures, no overwrite, argument and child-failure checks; no model/device')
