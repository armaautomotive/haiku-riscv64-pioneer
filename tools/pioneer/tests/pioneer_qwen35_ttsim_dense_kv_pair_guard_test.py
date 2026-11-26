#!/usr/bin/env python3
"""Reject K/V pair recorder images before the generic numerical runner starts."""
from pathlib import Path
import subprocess
import sys
import tempfile

runner = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
with tempfile.TemporaryDirectory() as temporary:
    image = Path(temporary) / 'image'
    image.mkdir()
    (image / 'dense-kv-pair.marker').touch()
    result = subprocess.run([sys.executable, '-B', str(runner), 'missing-library',
                             str(image), 'missing-roles'], capture_output=True, text=True)
    assert result.returncode != 0
    assert 'dense K/V pair image has no matching numerical simulator fixture' in result.stderr
print('PASS K/V pair image rejected before numerical simulator setup; OFFLINE ONLY')
