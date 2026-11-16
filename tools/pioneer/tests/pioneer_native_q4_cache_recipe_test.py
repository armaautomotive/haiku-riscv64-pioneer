#!/usr/bin/env python3
"""The exact reviewed reader-only source set preserves, but cannot widen, v1 cache identity."""
import pathlib
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(here))
from pioneer_native_q4_cache_recipe import ORIGINAL_RECIPE, REVIEWED_CURRENT_SOURCE, recipe, source_files

root, files = source_files()
assert recipe(root) == (REVIEWED_CURRENT_SOURCE, ORIGINAL_RECIPE)
with tempfile.TemporaryDirectory(prefix='pioneer-native-q4-recipe-') as directory:
    copied = pathlib.Path(directory)
    for name, data in files.items():
        target = copied / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    assert recipe(copied) == (REVIEWED_CURRENT_SOURCE, ORIGINAL_RECIPE)
    changed = copied / 'tools/pioneer/PioneerModelWeightCache.cpp'
    changed.write_bytes(changed.read_bytes() + b'\n// unreviewed mutation\n')
    try:
        recipe(copied)
    except ValueError:
        pass
    else:
        raise AssertionError('unreviewed cache source mutation was accepted')
print('PASS exact reader-only Q4 source identity preserves original cache recipe;mutation refused')
