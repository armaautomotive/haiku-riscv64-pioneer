#!/usr/bin/env python3
"""Build macOS-only weight preparers with distinct host-specific cache keys."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

if not __debug__ or sys.platform != 'darwin' or len(sys.argv) != 2:
    raise SystemExit('use NEW_ABSOLUTE_OUTPUT on macOS with assertions enabled')
if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('unexpected Pioneer environment override')
output = Path(sys.argv[1])
if not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('output must be a new absolute directory')
here = Path(__file__).resolve().parent
root = here.parent.parent
compiler = os.environ.get('CXX', 'c++')
machine = subprocess.check_output([compiler, '-dumpmachine'])
version = subprocess.check_output([compiler, '--version'])
if b'apple-darwin' not in machine:
    raise SystemExit('host preparer requires a macOS compiler target')
flags = ['-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
         '-DPIONEER_Q38_HOST_PREPARATION=1']
specs = (
    ('bfp4', 'pioneer_q38_hybrid_bfp4_prepare.cpp', 'PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC', 'PIONEER_BFP4_IMPLEMENTATION_SHA256'),
    ('head-bfp8', 'pioneer_q38_head_bfp8_prepare.cpp', 'PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC', 'PIONEER_HEAD_BFP8_IMPLEMENTATION_SHA256'),
)
output.mkdir(mode=0o700)
manifest = {}
for name, source, role, fingerprint_macro in specs:
    source_path = here / source
    role_flags = ['-D' + role + '=1', '-D' + fingerprint_macro + '="' + '0'*64 + '"']
    dependency_output = subprocess.check_output([compiler, *flags, *role_flags, '-MM', '-MT', 'q38',
                                                  str(source_path), str(here / 'PioneerModelWeightCache.cpp')], text=True)
    # Two compilation units each emit a q38 target. Hash every actual local
    # dependency, not just the small subset needed by an older target recipe.
    deps = set()
    for line in dependency_output.replace('\\\n', ' ').splitlines():
        tokens = shlex.split(line)
        if not tokens:
            continue
        if tokens[0] != 'q38:':
            raise RuntimeError('unexpected compiler dependency output')
        deps.update(Path(token).resolve() for token in tokens[1:])
    if not {source_path.resolve(), (here / 'PioneerModelWeightCache.cpp').resolve()} <= deps:
        raise RuntimeError('incomplete compiler dependency list')
    digest = hashlib.sha256(b'pioneer-q38-host-preparation-v1\0' + name.encode() + b'\0')
    for blob in (machine, version, '\0'.join(flags).encode()):
        digest.update(len(blob).to_bytes(8, 'little') + blob)
    for path in sorted(deps):
        if not path.is_relative_to(root):
            raise RuntimeError('dependency outside source tree: ' + str(path))
        label = str(path.relative_to(root)).encode()
        data = path.read_bytes()
        digest.update(len(label).to_bytes(8, 'little') + label + len(data).to_bytes(8, 'little') + data)
    fingerprint = digest.hexdigest()
    if fingerprint in ('5dc4ffb1257bf99a8b38f7139e5fe95d04e42f11d63beda382d78aeacb1dc45c',
                       'e0e45dce6f1d95124aaeb956e2ee4076aff5b11d35d8079e205054cf33cf316f'):
        raise RuntimeError('host key must never equal a Haiku target fingerprint')
    binary = output / ('pioneer-q38-host-' + name + '-prepare')
    command = [compiler, *flags, '-D' + role + '=1', '-D' + fingerprint_macro + '="' + fingerprint + '"',
               str(source_path), str(here / 'PioneerModelWeightCache.cpp'), '-o', str(binary)]
    subprocess.run(command, check=True, cwd=output)
    manifest[name] = {'fingerprint': fingerprint, 'sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                      'dependencies': [str(p.relative_to(root)) for p in sorted(deps)], 'command': command}
    print('HOST_PREPARER', name, 'fingerprint', fingerprint, 'sha256', manifest[name]['sha256'], flush=True)
(output / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
print('BUILD ONLY: host-specific keys, no model/cache/device access; not Haiku-qualified')
