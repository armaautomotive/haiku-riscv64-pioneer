#!/usr/bin/env python3
"""Source-frozen main/clean client qualification. NEVER access the Pioneer."""
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv) == 6
sdk, libraries, discovery, clean, out = map(Path, sys.argv[1:])
here = Path(__file__).resolve().parent
root = here.parents[2]
assert all(p.is_absolute() and p.is_dir() for p in (sdk, libraries, discovery, clean))
assert out.is_absolute() and not out.exists() and not out.is_symlink() and not out.is_relative_to(root)
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700)
public = set()
for line in (discovery/'source-sha256.txt').read_text().splitlines():
    public.add(root/line.split('  ', 1)[1])
host = os.environ.get('CXX', 'c++')
flags = ['-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1', '-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1',
         '-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1', '-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1']
for kind in ('protocol', 'session'):
    data = subprocess.check_output([host, '-std=c++17', *flags, '-MM',
                                    str(here/('pioneer_q38_full_offload_'+kind+'_test.cpp'))], text=True)
    for line in data.replace('\\\n', '').splitlines():
        for name in line.split(':', 1)[1].split():
            p = Path(name).resolve()
            if p.is_relative_to(root):
                public.add(p)
def collect(path):
    path = path.resolve()
    if path in public:
        return
    public.add(path)
    if path.suffix != '.py':
        return
    for node in ast.walk(ast.parse(path.read_text())):
        names = []
        if isinstance(node, ast.Import):
            names += [n.name+'.py' for n in node.names]
        if isinstance(node, ast.ImportFrom) and node.module:
            names += [node.module+'.py']
        if isinstance(node, ast.Constant) and isinstance(node.value, str) and node.value.endswith(('.py', '.sh', '.cpp', '.h')):
            names += [node.value]
        for name in names:
            for parent in (path.parent, path.parent.parent):
                p = parent/name
                if p.is_file() and p.resolve().is_relative_to(root):
                    collect(p)
for name in ('run_pioneer_q38_fast_decode_client_tests.py', 'run_pioneer_q38_fast_decode_client_session_tests.py',
             'pioneer_q38_fast_decode_profile_log_test.py', 'pioneer_q38_fast_decode_profile_parent_test.py',
             'pioneer_q38_full_offload_phase_profile_log_test.py', 'pioneer_q38_bf16_profile_rate_log_test.py',
             'pioneer_q38_startup_timing_log_test.py', 'run_pioneer_q38_full_offload_rate_tests.py'):
    collect(here/name)
collect(here.parent/'pioneer_q38_fast_decode_profile_physical.sh')
public.add(here.parent/'build_assistant_qwen27_fast_decode_full_offload.py')
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
relative = sorted(str(p.relative_to(root)) for p in public)
frozen = {label: {p: sha(source/p) for p in relative} for label, source in (('main', root), ('clean', clean))}
allowed = {'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadMetricsRole.h',
           'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h'}
differences = {p for p in relative if frozen['main'][p] != frozen['clean'][p]}
assert differences <= allowed, differences
library_pins = {p.name: sha(p) for p in libraries.glob('*.so.*') if p.is_file()}
assert len(library_pins) == 4
def same():
    for label, source in (('main', root), ('clean', clean)):
        assert all(sha(source/p) == digest for p, digest in frozen[label].items()), label
    assert all(sha(libraries/p) == digest for p, digest in library_pins.items())
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
    assert result.returncode == 0 and not (out/(name+'.stderr')).stat().st_size, name
    print('CLOSED_FAST_DECODE_CLIENT_LOCAL', name, result.returncode, flush=True)
for label, source in (('main', root), ('clean', clean)):
    t = source/'tools/pioneer'
    tests = t/'tests'
    run(label+'-build', [sys.executable, t/'build_assistant_qwen27_fast_decode_full_offload.py', sdk, libraries, out/(label+'-build')])
    for kind, name in (('session', 'run_pioneer_q38_fast_decode_client_session_tests.py'),
                       ('log', 'pioneer_q38_fast_decode_profile_log_test.py'),
                       ('parent', 'pioneer_q38_fast_decode_profile_parent_test.py')):
        run(label+'-'+kind, [sys.executable, tests/name])
    for name in ('pioneer_q38_full_offload_phase_profile_log_test.py', 'pioneer_q38_bf16_profile_rate_log_test.py',
                 'pioneer_q38_startup_timing_log_test.py'):
        run(label+'-'+name, [sys.executable, tests/name])
    run(label+'-rate-process', [sys.executable, tests/'run_pioneer_q38_full_offload_rate_tests.py', '--startup-timing'])
same()
for name in ('HaikuAssistantQwen27FastDecodeFullOffload', 'pioneer-q38-fast-decode-rate-smoke'):
    assert (out/'main-build'/name).read_bytes() == (out/'clean-build'/name).read_bytes(), name
for kind in ('session', 'log', 'parent', 'rate-process'):
    assert (out/('main-'+kind+'.stdout')).read_bytes() == (out/('clean-'+kind+'.stdout')).read_bytes(), kind
for label in ('main', 'clean'):
    (out/(label+'-source-sha256.txt')).write_text(''.join(digest+'  '+p+'\n' for p, digest in frozen[label].items()))
(out/'libraries-sha256.txt').write_text(''.join(digest+'  '+p+'\n' for p, digest in sorted(library_pins.items())))
(out/'preexisting-differences.json').write_text(json.dumps(sorted(differences), indent=2)+'\n')
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
print('PASS_FAST_DECODE_CLIENT_LOCAL children'+str(len(exits))+'_stderr0_sourcesPerTree'+str(len(relative))+
      '_mainCleanWorkerAndClientExact_originalVector92Exact;NO_HARDWARE_MODEL_QUALITY_OR_SPEED_RESULT', flush=True)
