#!/usr/bin/env python3
"""Compare retained 6144-float attention input data; never access a card."""
import argparse
import hashlib
import math
import os
from pathlib import Path
import re
import sys
from compare_pioneer_q38_observed_trace import difference, require, stable_bytes
from compare_pioneer_q38_qgw8_dense_stage_trace import projection_diagnostics, unpack

REFERENCE_HASH = '972b0747f8b03d422a7c5315ccd0fd15bf227231b1c316d5e7026c53eaf0ebf0'
BASELINE_HASH = '103312ce324384782d322b51a457d654c4c5c1fef181ceecc730aa6a3d82af2e'
LAYER_WORDS, INPUT_WORDS, HEAD_WORDS = 5120, 6144, 256

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('reference', type=Path)
    p.add_argument('baseline', type=Path)
    p.add_argument('candidate', type=Path)
    p.add_argument('--candidate-sha256', required=True)
    args = p.parse_args()
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'role override')
    require(re.fullmatch(r'[0-9a-f]{64}', args.candidate_sha256) is not None, 'candidate hash')
    entries = ((args.reference, INPUT_WORDS*4, REFERENCE_HASH),
               (args.baseline, 64*LAYER_WORDS*4, BASELINE_HASH),
               (args.candidate, 64*LAYER_WORDS*4, args.candidate_sha256))
    data = []
    for path, size, digest in entries:
        require(path.is_absolute() and not path.is_symlink(), 'absolute nonsymlink input required')
        raw = stable_bytes(path,size)
        require(hashlib.sha256(raw).hexdigest() == digest, 'input hash mismatch')
        data.append(unpack(raw))
    reference, baseline, candidate = data
    for label, offset, width in [('all',0,INPUT_WORDS)] + [(f'head={i}',i*HEAD_WORDS,HEAD_WORDS) for i in range(24)]:
        changed, maximum, relative = difference(reference,candidate,offset,width)
        ref_norm, card_norm, gain, cosine = projection_diagnostics(reference,candidate,offset,width)
        require(all(math.isfinite(v) for v in (maximum,relative,ref_norm,card_norm)), 'invalid result')
        show = lambda value: 'undefined' if value is None else f'{value:.12g}'
        print(f'input {label} changed={changed} max_abs={maximum:.12g} relative_l2={relative:.12g} '
              f'cpu_l2={ref_norm:.12g} card_l2={card_norm:.12g} fit={show(gain)} cosine={show(cosine)}')
    for layer in range(2,64):
        changed, maximum, relative = difference(baseline,candidate,layer*LAYER_WORDS,LAYER_WORDS)
        print(f'layer={layer} changed={changed} max_abs={maximum:.12g} relative_l2={relative:.12g}')
    print('RAW DATA ONLY: hash does not prove source, completion, capture identity, correctness or speed')

if __name__ == '__main__':
    try:
        main()
    except (ValueError,OSError) as error:
        print('DENSE_INPUT_COMPARISON_REFUSED ' + str(error),file=sys.stderr)
        sys.exit(2)
