#!/usr/bin/env python3
"""Synthetic tests only; no card or model access."""
import array
import contextlib
import hashlib
import io
from pathlib import Path
import sys
import tempfile
import compare_pioneer_q38_qgw8_dense_stage_trace as subject

with tempfile.TemporaryDirectory(prefix='qgw8-dense-stage-compare-') as tmp:
    root = Path(tmp)
    ref, base, candidate = root/'ref', root/'base', root/'candidate'
    reference = array.array('f', [1.0]*(subject.WIDTH*2))
    baseline = array.array('f', [1.0]*(subject.WIDTH*64))
    altered = array.array('f', baseline)
    altered[0] = 2.0
    altered[subject.WIDTH] = 3.0
    altered[3*subject.WIDTH] = 4.0
    ref.write_bytes(reference.tobytes()); base.write_bytes(baseline.tobytes()); candidate.write_bytes(altered.tobytes())
    subject.REFERENCE_HASH = hashlib.sha256(ref.read_bytes()).hexdigest()
    subject.BASELINE_HASH = hashlib.sha256(base.read_bytes()).hexdigest()
    digest = hashlib.sha256(candidate.read_bytes()).hexdigest()
    sys.argv = ['compare', str(ref), str(base), str(candidate), '--candidate-sha256', digest]
    stream = io.StringIO()
    with contextlib.redirect_stdout(stream): subject.main()
    output = stream.getvalue()
    assert 'projection bit_differences=1' in output and 'residual bit_differences=1' in output
    assert 'layer=2 changed=0' in output and 'layer=3 changed=1' in output and 'layer=63 changed=0' in output
    sys.argv[-1] = '0'*64
    try: subject.main()
    except ValueError: pass
    else: raise AssertionError('wrong candidate digest accepted')
    altered[0] = float('nan'); candidate.write_bytes(altered.tobytes())
    sys.argv[-1] = hashlib.sha256(candidate.read_bytes()).hexdigest()
    try: subject.main()
    except ValueError: pass
    else: raise AssertionError('nonfinite candidate accepted')
print('PASS synthetic stage/layer offsets, hashes and finite-value refusals; NO DEVICE')
