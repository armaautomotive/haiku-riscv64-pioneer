#!/usr/bin/env python3
"""Compatibility is allowed only for the reviewed exact converter inputs."""
import importlib.util
from pathlib import Path
import shutil
import tempfile

source = Path(__file__).resolve().parent.parent
names = ('PioneerQ38LosslessWeightCache.h', 'PioneerQwen38TiledReader.h',
         'PioneerQwen38PackedPlan.h', 'PioneerQwen38Gguf.h',
         'TenstorrentGgufTensor.h', 'TenstorrentGgufQuant.h', 'TenstorrentQ4K.h',
         'PioneerAttentionMath.h', 'PioneerModelWeightCache.h',
         'PioneerModelWeightCache.cpp', 'qwen38_lossless_cache_recipe.py')
expected = 'bb0e60700ff8345b8ea493c7c8e178c5400c3141969cb61ea74693139721d25a'
with tempfile.TemporaryDirectory(prefix='q38-cache-recipe-', dir='/private/tmp') as tmp:
    directory = Path(tmp)
    for name in names:
        shutil.copyfile(source / name, directory / name)
    spec = importlib.util.spec_from_file_location('recipe_test', directory / names[-1])
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    assert module.fingerprint() == expected
    for name in names:
        path = directory / name
        original = path.read_bytes()
        path.write_bytes(original + b'\n')
        assert module.fingerprint() != expected, name
        path.write_bytes(original)
    assert module.fingerprint() == expected
print('PASS exact reviewed cache compatibility and all eleven source-change refusals; NO DEVICE')
