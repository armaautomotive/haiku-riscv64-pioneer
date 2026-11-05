#!/usr/bin/env python3
"""Check saved offline DATA/source closure;never device or execution authority."""
import hashlib
import json
import os
from pathlib import Path
import re
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
assert len(sys.argv) == 1
here = Path(__file__).resolve().parent
root = here.parents[3]
main, clean = here / 'main', here / 'clean'
expected = {'build', 'metadata-compile', 'metadata', 'pipeline-compile', 'pipeline', 'image-compile', 'image',
            'old-pipeline-compile', 'old-pipeline', 'old-image-compile', 'old-image', 'model-compile', 'model', 'fraction', 'old-fraction'}
exits = json.loads((main / 'actual-exits.json').read_text())
assert set(exits) == expected and set(exits.values()) == {0}
assert (main / 'actual-exits.json').read_bytes() == (clean / 'actual-exits.json').read_bytes()
for name in expected:
    for suffix in ('.stdout', '.stderr'):
        data = (main / (name + suffix)).read_bytes()
        assert data == (clean / (name + suffix)).read_bytes(), name + suffix
        if suffix == '.stderr':
            assert not data
for name in ('parent-main.stderr', 'parent-clean.stderr'):
    assert not (here / name).read_bytes()
assert (here / 'parent-main.stdout').read_bytes() == (here / 'parent-clean.stdout').read_bytes()
assert (main / 'source-sha256.txt').read_bytes() == (clean / 'source-sha256.txt').read_bytes()
seen = set()
for line in (main / 'source-sha256.txt').read_text().splitlines():
    digest, name = line.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and name not in seen
    path = root / name
    assert path.resolve().is_relative_to(root) and path.is_file() and not path.is_symlink()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, name
    seen.add(name)
assert len(seen) == 50
assert (main / 'role-refusals.txt').read_text() == (clean / 'role-refusals.txt').read_text() == 'PASS_COMPACT_IMAGE_ROLE_REFUSALS 25 definedZeroIncluded_NO_NATIVE_OR_WORK\n'
image = (main / 'image.stdout').read_text()
assert image.endswith('PASS_COMPACT_ACTUAL_RV32 cases8_twoJobs_each64WordsExact controls7 numerical8 original8KiBROM_8KiBStack_bounds SP96 touched84;SAME_RAW_BYTES_UNPACK_SFPU_BF16_MOVD2A_4PHASE_FPU_NO_SCALAR_PAYLOAD;NO_NATIVE_MODEL_QUALITY_OR_SPEED\n')
model = (main / 'model.stdout').read_text()
assert len(re.findall(r'^PASS_PRIVATE_COMPACT_FPU_ACTUAL ', model, re.M)) == 193
assert model.endswith('PASS_PRIVATE_COMPACT_FPU_ORIGINAL tensors193 sampledBlocks1737 actualJobs3474 returnedWords111168 sameOpenFullSHA_epochAndIdentityStable;diagnosticBF16_inputs_notRealGenerationQualityOrSpeed\n')
print('PASS_COMPACT_ARCHIVED_EVIDENCE children15_streamsExact_stderr0_source50_currentAndCleanExact_193OriginalQ4Tensors;NOT_DEVICE_NATIVE_MODEL_QUALITY_OR_SPEED_AUTHORITY')
