#!/usr/bin/env python3
"""Complete lossless-cache source fingerprint, shared by CLI and Assistant builds."""
import hashlib
import pathlib

def fingerprint():
    here = pathlib.Path(__file__).resolve().parent
    inputs = ('PioneerQ38LosslessWeightCache.h', 'PioneerQwen38TiledReader.h',
              'PioneerQwen38PackedPlan.h', 'PioneerQwen38Gguf.h',
              'TenstorrentGgufTensor.h', 'TenstorrentGgufQuant.h', 'TenstorrentQ4K.h',
              'PioneerAttentionMath.h', 'PioneerModelWeightCache.h',
              'PioneerModelWeightCache.cpp', 'qwen38_lossless_cache_recipe.py')
    digest = hashlib.sha256()
    for name in inputs:
        digest.update(name.encode() + b'\0' + (here / name).read_bytes() + b'\0')
    return digest.hexdigest()

if __name__ == '__main__':
    print(fingerprint())
