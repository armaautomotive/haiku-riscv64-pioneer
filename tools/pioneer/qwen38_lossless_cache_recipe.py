#!/usr/bin/env python3
"""Complete lossless-cache source fingerprint, shared by CLI and Assistant builds."""
import hashlib
import pathlib
import re

# Digest of this script with the value on this line replaced by an empty
# string. This prevents an unrelated later script edit retaining the alias.
_SELF_HASH = 'fb6ce9b118351f99952ae4d9b643fc0e90eb7d20ef2792b26dc54dae16bf2f27'

def fingerprint():
    here = pathlib.Path(__file__).resolve().parent
    inputs = ('PioneerQ38LosslessWeightCache.h', 'PioneerQwen38TiledReader.h',
              'PioneerQwen38PackedPlan.h', 'PioneerQwen38Gguf.h',
              'TenstorrentGgufTensor.h', 'TenstorrentGgufQuant.h', 'TenstorrentQ4K.h',
              'PioneerAttentionMath.h', 'PioneerModelWeightCache.h',
              'PioneerModelWeightCache.cpp', 'qwen38_lossless_cache_recipe.py')
    # Exact, reviewed reader-only changes: the converter and cache wire
    # format are unchanged from the installed November 3 worker. Preserve
    # its cache identity only for this precisely pinned set of input bytes;
    # any future source edit falls back to a fresh full fingerprint below.
    compatible = hashlib.sha256()
    for name in inputs[:-1]:
        compatible.update(name.encode() + b'\0' + (here / name).read_bytes() + b'\0')
    script = (here / inputs[-1]).read_bytes()
    normalized, count = re.subn(rb"(?m)^_SELF_HASH = '[0-9a-f]*'$", b"_SELF_HASH = ''", script)
    if (count == 1 and hashlib.sha256(normalized).hexdigest() == _SELF_HASH
            and compatible.hexdigest() == '705254577615fdabae157891281620bd19c40b3a54af521c49c94b61637fa17c'):
        return 'bb0e60700ff8345b8ea493c7c8e178c5400c3141969cb61ea74693139721d25a'
    digest = hashlib.sha256()
    for name in inputs:
        digest.update(name.encode() + b'\0' + (here / name).read_bytes() + b'\0')
    return digest.hexdigest()

if __name__ == '__main__':
    print(fingerprint())
