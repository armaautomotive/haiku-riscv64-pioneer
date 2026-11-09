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
    card[0]=2.0;card[256]=3.0;card[512]=0.0;card[6143]=4.0;card[2*subject.LAYER_WORDS]=5.0
    reference.write_bytes(ref.tobytes());baseline.write_bytes(base.tobytes());candidate.write_bytes(card.tobytes())
    subject.REFERENCE_HASH=hashlib.sha256(reference.read_bytes()).hexdigest()
    subject.BASELINE_HASH=hashlib.sha256(baseline.read_bytes()).hexdigest()
    sys.argv=['compare',str(reference),str(baseline),str(candidate),'--candidate-sha256',hashlib.sha256(candidate.read_bytes()).hexdigest()]
    stream=io.StringIO()
    with contextlib.redirect_stdout(stream):subject.main()
    output=stream.getvalue()
    assert 'input all changed=4' in output and 'input head=0 changed=1' in output
    assert 'input head=1 changed=1' in output and 'input head=23 changed=1' in output
    assert 'layer=2 changed=1' in output and 'layer=3 changed=0' in output and 'layer=63 changed=0' in output
    assert 'cross_head card=0 best_cpu=' in output and 'zero_lanes card=0 count=0' in output
    assert 'zero_lanes card=2 count=1 per_mod16=1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0' in output
    assert 'not a permutation or cause proof' in output
    sys.argv[-1]='0'*64
    try:subject.main()
    except ValueError:pass
    else:raise AssertionError('wrong digest accepted')
    card[0]=float('nan');candidate.write_bytes(card.tobytes());sys.argv[-1]=hashlib.sha256(candidate.read_bytes()).hexdigest()
    try:subject.main()
    except ValueError:pass
    else:raise AssertionError('nonfinite capture accepted')

# Distinct orthogonal heads expose a signed cross-head match; zero input has
# no defined cosine and ties deterministically select the first reference.
refs=array.array('f',[0.0]*subject.INPUT_WORDS)
vals=array.array('f',[0.0]*subject.INPUT_WORDS)
for head in range(24):refs[head*subject.HEAD_WORDS+head]=1.0
vals[3*subject.HEAD_WORDS+7]=-2.0
assert subject.closest_reference_head({'floats':refs},{'floats':vals},3)==(7,-1.0)
assert subject.closest_reference_head({'floats':refs},{'floats':vals},4) is None
refs[8*subject.HEAD_WORDS+8]=0.0;refs[8*subject.HEAD_WORDS+7]=1.0
assert subject.closest_reference_head({'floats':refs},{'floats':vals},3)==(7,-1.0)
print('PASS synthetic input/head/layer offsets, cross-head signed/zero/tie cases, hash and finite-value refusals; NO DEVICE')
