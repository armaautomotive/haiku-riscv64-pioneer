#!/usr/bin/env python3
"""Resolve only the original or exactly reviewed reader-only Q4 cache sources."""
import hashlib
import pathlib
import re

ORIGINAL_RECIPE = '70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6'
REVIEWED_CURRENT_SOURCE = '872f65f457a30c935aeec6e2701e0dc145b8065a71c915a46f41d5b40907698e'


def source_files(root=None):
    here = pathlib.Path(__file__).resolve().parent
    root = pathlib.Path(root).resolve() if root else here.parent.parent
    sources = {}

    def collect(path):
        assert path.is_file() and not path.is_symlink()
        name = path.relative_to(root).as_posix()
        if name in sources:
            return
        data = path.read_bytes()
        sources[name] = data
        for include in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"', data.decode(), re.M):
            candidate = (path.parent / include).resolve()
            candidate.relative_to(root)
            assert not candidate.is_symlink()
            collect(candidate)

    source_dir = root / 'tools/pioneer'
    for name in ('pioneer_q38_native_q4_prepare.cpp', 'PioneerModelWeightCache.cpp'):
        collect(source_dir / name)
    return root, sources


def source_fingerprint(root=None):
    root, sources = source_files(root)
    digest = hashlib.sha256()
    for name, data in sorted(sources.items()):
        digest.update(name.encode() + b'\0' + len(data).to_bytes(8, 'little') + data)
    return digest.hexdigest()


def recipe(root=None):
    source = source_fingerprint(root)
    if source not in (ORIGINAL_RECIPE, REVIEWED_CURRENT_SOURCE):
        raise ValueError('unreviewed native-Q4 cache sources; do not change the persistent cache identity')
    return source, ORIGINAL_RECIPE


if __name__ == '__main__':
    source, identity = recipe()
    print('SOURCE_FINGERPRINT ' + source)
    print('UNCHANGED_CACHE_RECIPE ' + identity)
