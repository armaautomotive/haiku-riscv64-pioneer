#!/usr/bin/env python3
"""Resolve only the original or exactly reviewed reader-only Q4 cache sources."""
import hashlib
import pathlib
import re

ORIGINAL_RECIPE = '70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6'
# The diagnostic-only prepared-Q4 reader selects OpenExisting for the
# manifests and bundles. Normal builds still use GetOrCreate, and the
# conversion recipe identity below intentionally remains unchanged.
REVIEWED_READ_ONLY_SOURCE = 'bc4adf382e35955f598e599f189819b0c2f3ede6acfc26ed756ddb1a7ad688c4'
# The payload reader reuses an in-memory verification receipt. It does not
# change the manifest, converter, payload/index, padding or on-disk layout.
REVIEWED_CURRENT_SOURCE = '686b96218fdf18ec22aad266635789c0ad5124b764b507b05e0cc29a8b398049'


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
    if source not in (ORIGINAL_RECIPE, REVIEWED_READ_ONLY_SOURCE, REVIEWED_CURRENT_SOURCE):
        raise ValueError('unreviewed native-Q4 cache sources; do not change the persistent cache identity')
    return source, ORIGINAL_RECIPE


if __name__ == '__main__':
    source, identity = recipe()
    print('SOURCE_FINGERPRINT ' + source)
    print('UNCHANGED_CACHE_RECIPE ' + identity)
