#!/usr/bin/env python3
"""Fail early if a source edit breaks the frozen existing BFP8 cache key."""
import ast
import hashlib
from pathlib import Path

if not __debug__:
    raise SystemExit('assertions are required')
root = Path(__file__).resolve().parent.parent
builder = root / 'build_qwen38_native_weight_cache.py'
tree = ast.parse(builder.read_text())
assignments = [node for node in tree.body if isinstance(node, ast.Assign)
               and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name)
               and node.targets[0].id == 'inputs']
assert len(assignments) == 1
inputs = ast.literal_eval(assignments[0].value)
assert isinstance(inputs, tuple) and len(inputs) == 11
assert 'PioneerModelWeightCache.cpp' in inputs
inputs += ('PioneerQ38NativeBfp8WeightTiles.h',)
digest = hashlib.sha256(b'all-bfp8\0')
for name in inputs:
    source = root / 'frozen_bfp8' / name if name in ('PioneerModelWeightCache.h', 'PioneerModelWeightCache.cpp') else root / name
    digest.update(name.encode() + b'\0' + source.read_bytes() + b'\0')
fingerprint = digest.hexdigest()
assert fingerprint == '7db613bfdde463be7f933f25123a00dbe39928cbd8a070aa2481c6cb936753d0', \
    'Frozen BFP8 converter inputs changed; existing caches require an explicit compatibility plan'
recipe = ('pioneer-native-weight-tiles-v2-all-bfp8/outputs32-K32/faces16-row-major/zero-padding/'
          'Q4_K,Q8_0,Q6_K-to-BFP8_B/'
          'shared-exp-FP32-bias/align-truncate-then-RNE-saturate/'
          'no-subnormal-input/source-fingerprint=' + fingerprint)
key = hashlib.sha256(recipe.encode()).hexdigest()
assert key == '551c6713a31c749d0dad82e616c592f01aba6854af8781f6756b4f60d85bc897'
assert key in (root / 'PioneerQ38NativeBfpBandSource.h').read_text()
frozen_header = (root / 'frozen_bfp8' / 'PioneerModelWeightCache.h').read_text()
wrapper = (root / 'frozen_bfp8' / 'pioneer_q38_hybrid_bfp8_frozen.cpp').read_text()
assert 'OpenExistingDirectory' not in frozen_header
assert 'OpenExistingDirectory' in (root / 'PioneerModelWeightCache.h').read_text()
assert wrapper.index('#include "PioneerModelWeightCache.h"') < wrapper.index('#include "../pioneer_q38_hybrid_bfp8_prepare.cpp"')
print('PASS frozen BFP8 writer source fingerprint and reader recipe pin; no model or device access')
