#!/usr/bin/env python3
"""Synthetic offline comparison tests; no model or device."""
import array
import contextlib
import hashlib
import io
from pathlib import Path
import sys
import tempfile
import compare_pioneer_q38_qgw8_dense_input_trace as subject

with tempfile.TemporaryDirectory(prefix='qgw8-input-compare-') as tmp:
    root=Path(tmp); reference=root/'ref'; baseline=root/'base'; candidate=root/'candidate'
    ref=array.array('f',[1.0]*subject.INPUT_WORDS)
    base=array.array('f',[1.0]*(64*subject.LAYER_WORDS)); card=array.array('f',base)
    card[0]=2.0;card[256]=3.0;card[6143]=4.0;card[2*subject.LAYER_WORDS]=5.0
    reference.write_bytes(ref.tobytes());baseline.write_bytes(base.tobytes());candidate.write_bytes(card.tobytes())
    subject.REFERENCE_HASH=hashlib.sha256(reference.read_bytes()).hexdigest()
    subject.BASELINE_HASH=hashlib.sha256(baseline.read_bytes()).hexdigest()
    sys.argv=['compare',str(reference),str(baseline),str(candidate),'--candidate-sha256',hashlib.sha256(candidate.read_bytes()).hexdigest()]
    stream=io.StringIO()
    with contextlib.redirect_stdout(stream):subject.main()
    output=stream.getvalue()
    assert 'input all changed=3' in output and 'input head=0 changed=1' in output
    assert 'input head=1 changed=1' in output and 'input head=23 changed=1' in output
    assert 'layer=2 changed=1' in output and 'layer=3 changed=0' in output and 'layer=63 changed=0' in output
    sys.argv[-1]='0'*64
    try:subject.main()
    except ValueError:pass
    else:raise AssertionError('wrong digest accepted')
    card[0]=float('nan');candidate.write_bytes(card.tobytes());sys.argv[-1]=hashlib.sha256(candidate.read_bytes()).hexdigest()
    try:subject.main()
    except ValueError:pass
    else:raise AssertionError('nonfinite capture accepted')
print('PASS synthetic input/head/layer offsets, hash and finite-value refusals; NO DEVICE')
