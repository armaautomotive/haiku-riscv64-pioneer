#!/usr/bin/env python3
"""Freeze source, then main/clean diagnostic native checks. NO device IO."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv) == 6
sdk, image, discovery, clean, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
root = here.parents[2]
assert all(p.is_absolute() and p.is_dir() for p in (sdk, image, discovery, clean))
assert out.is_absolute() and not out.exists() and not out.is_symlink() and not out.is_relative_to(root)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700)
host = os.environ.get('CXX', 'c++')
relative = set()
for dependency in discovery.rglob('*.d'):
    data = dependency.read_text().replace('\\\n', '')
    for line in data.splitlines():
        if ':' not in line:
            continue
        for item in line.split(':', 1)[1].split():
            p = Path(item).resolve()
            if p.is_relative_to(root):
                relative.add(str(p.relative_to(root)))
# Host fixture dependencies include the actual complete memory algorithm;
# the definition file contains only the already-tested fixed role selectors.
define_dir = discovery.parent/'main-data'
text = subprocess.check_output([host, '-std=c++17', '-I'+str(define_dir), '-MM',
                               str(here/'pioneer_q38_fast_decode_native_memory_test.cpp')], text=True)
for rule in text.replace('\\\n', '').splitlines():
    if ':' not in rule:
        continue
    for item in rule.split(':', 1)[1].split():
        p = Path(item).resolve()
        if p.is_relative_to(root):
            relative.add(str(p.relative_to(root)))
for name in ('build_qwen38_fast_decode_profile_native.py', 'build_qwen38_full_offload_native.py',
             'generate_qwen38_fast_decode_profile_programs.py', 'PioneerQ38TokenWideGdnPrograms.h'):
    relative.add('tools/pioneer/'+name)
for name in ('run_pioneer_q38_fast_decode_native_tests.py', 'run_pioneer_q38_fast_decode_native_data_tests.py',
             'pioneer_q38_fast_decode_native_data_test.cpp', 'pioneer_q38_fast_decode_profile_metric_fixture.h',
             'run_pioneer_q38_token_phase_profile_read_tests.py', 'run_pioneer_q38_token_phase_profile_control_tests.py',
             'pioneer_q38_token_phase_profile_read_test.cpp', 'pioneer_q38_token_phase_profile_control_test.cpp'):
    relative.add('tools/pioneer/tests/'+name)
# These private -isystem inputs are intentionally absent from -MMD output.
for base in ('headers', 'build/config_headers'):
    relative.update(str(p.relative_to(root)) for p in (root/base).rglob('*') if p.is_file())
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
frozen = {label: {p: sha(source/p) for p in sorted(relative)} for label, source in (('main', root), ('clean', clean))}
allowed = {'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadMetricsRole.h',
           'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h',
           'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h',
           'src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc',
           'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeEnd.inc',
           'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp',
           'tools/pioneer/build_qwen38_full_offload_native.py'}
differences = {p for p in relative if frozen['main'][p] != frozen['clean'][p]}
assert differences <= allowed, differences
images = {p.name: sha(p) for p in image.iterdir() if p.name in
          ('boot.bin', 'text.bin', 'terminal.bin', 'conv.bin', 'fast_decode.bin', 'program.elf')}
assert len(images) == 6
def same():
    for label, source in (('main', root), ('clean', clean)):
        assert all(sha(source/p) == digest for p, digest in frozen[label].items()), label
    assert all(sha(image/name) == digest for name, digest in images.items())
exits = {}
commands = []
def run(name, command):
    same()
    command = list(map(str, command))
    commands.append([name, *command])
    with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
        result = subprocess.run(command, stdout=stdout, stderr=stderr, cwd=out,
                                env={**os.environ, 'ASAN_OPTIONS': 'quarantine_size_mb=32',
                                     'UBSAN_OPTIONS': 'halt_on_error=1', 'PYTHONDONTWRITEBYTECODE': '1'})
    exits[name] = result.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2)+'\n')
    assert result.returncode == 0 and (out/(name+'.stderr')).stat().st_size == 0, name
    print('CLOSED_FAST_DECODE_NATIVE_LOCAL', name, result.returncode, flush=True)
for label, source in (('main', root), ('clean', clean)):
    t = source/'tools/pioneer'
    tests = t/'tests'
    run(label+'-build', [sys.executable, t/'build_qwen38_fast_decode_profile_native.py', sdk, out/(label+'-build')])
    run(label+'-data', [sys.executable, tests/'run_pioneer_q38_fast_decode_native_data_tests.py', image, out/(label+'-data')])
    for kind in ('read', 'control'):
        run(label+'-'+kind, [sys.executable, tests/('run_pioneer_q38_token_phase_profile_'+kind+'_tests.py'), '--fast-decode-profile'])
    binary = out/(label+'-memory-test')
    run(label+'-memory-compile', [host, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                                '-fsanitize=address,undefined', '-I'+str(out/(label+'-data')),
                                tests/'pioneer_q38_fast_decode_native_memory_test.cpp', '-o', binary])
    run(label+'-memory', [binary])
same()
assert (out/'main-build/tenstorrent_pci_probe-fast-decode116').read_bytes() == \
       (out/'clean-build/tenstorrent_pci_probe-fast-decode116').read_bytes()
for kind in ('read', 'control', 'memory'):
    assert (out/('main-'+kind+'.stdout')).read_bytes() == (out/('clean-'+kind+'.stdout')).read_bytes(), kind
for label in ('main', 'clean'):
    (out/(label+'-source-sha256.txt')).write_text(''.join(digest+'  '+p+'\n' for p, digest in frozen[label].items()))
(out/'image-sha256.txt').write_text(''.join(digest+'  '+name+'\n' for name, digest in sorted(images.items())))
(out/'preexisting-differences.json').write_text(json.dumps(sorted(differences), indent=2)+'\n')
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
print('PASS_FAST_DECODE_NATIVE_LOCAL children'+str(len(exits))+'_stderr0_sourcesPerTree'+str(len(relative))+
      '_mainCleanNativeModuleExact_legacyVector92AndDefault43Exact;NO_DEVICE_MODEL_SPEED_OR_ASSISTANT_QUALIFICATION', flush=True)
