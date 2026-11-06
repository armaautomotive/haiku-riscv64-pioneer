#!/usr/bin/env python3
"""Frozen local qualification of111/113/NEW114; NO staging/device access."""
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import sys
configured = sys.argv[-1:] == ['--configured']
manual = configured or sys.argv[-1:] == ['--manual-last']
if manual: sys.argv.pop()
assert __debug__ and len(sys.argv) == 6 and not os.environ.get('PYTHONOPTIMIZE')
sdk, private, original, out = map(Path, sys.argv[1:5]); generation = int(sys.argv[5])
assert 2 <= generation < 2**63
assert all(p.is_absolute() for p in (sdk, private, original, out)) and sdk.is_dir() and private.is_dir() and original.is_file()
assert not out.exists() and not out.is_symlink() and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = Path(__file__).resolve().parent; root = here.parents[2]
assert not out.is_relative_to(root)
role = 114 if configured else 113 if manual else 111
client_name = 'pioneer-native-factored-q4-configured' if configured else 'pioneer-native-factored-q4-manual-last' if manual else 'pioneer-native-factored-q4-compare'
client_selector = ['--configured'] if configured else ['--manual-last'] if manual else []
native_selector = '--configured-compare' if configured else '--manual-last-compare' if manual else '--factored-compare'
role_flags = [*(['-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=1'] if manual else []),
              *(['-DPIONEER_NATIVE_FACTORED_Q4_CONFIGURED=1'] if configured else [])]
# Broad pre/post snapshot; archive only actual consumed project sources below.
suffixes = {'.h', '.hpp', '.cpp', '.c', '.inc', '.py', '.sh', '.S', '.ld'}
frozen = {p: hashlib.sha256(p.read_bytes()).hexdigest() for folder in
          (root/'tools/pioneer', root/'src/add-ons/kernel', root/'headers/private', root/'build/config_headers')
          for p in folder.rglob('*') if p.is_file() and p.suffix in suffixes}
private_pins = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in private.glob('PioneerNativeCompactQ4Stream*.h')}
out.mkdir(mode=0o700); exits = {}; commands = []
def same():
    assert all(hashlib.sha256(p.read_bytes()).hexdigest() == v for p, v in frozen.items())
    assert all(hashlib.sha256((private/n).read_bytes()).hexdigest() == v for n, v in private_pins.items())
def run(name, args):
    same(); commands.append([name, *map(str, args)])
    with (out/(name+'.stdout')).open('xb') as stdout, (out/(name+'.stderr')).open('xb') as stderr:
        result = subprocess.run(list(map(str, args)), stdout=stdout, stderr=stderr,
                                env=dict(os.environ, ASAN_OPTIONS='quarantine_size_mb=32', UBSAN_OPTIONS='halt_on_error=1'))
    exits[name] = result.returncode
    (out/'actual-exits.json').write_text(json.dumps(exits, sort_keys=True, indent=2)+'\n')
    assert result.returncode == 0 and not (out/(name+'.stderr')).stat().st_size, name
    print('CLOSED_COMPARE'+str(role)+'_LOCAL', name, result.returncode, flush=True)
run('native-build', [sys.executable, here.parent/'build_native_compact_q4_stream_diagnostic.py',
                     sdk, private, original, out/'native', native_selector])
run('client-build', [sys.executable, here.parent/'build_native_factored_q4_compare_client.py',
                     sdk, private, out/'client', generation, *client_selector])
parent_path = here.parent/('pioneer_native_factored_q4_configured_physical.sh' if configured else
                          'pioneer_native_factored_q4_manual_last_physical.sh' if manual else
                          'pioneer_native_factored_q4_compare_physical.sh')
parent = parent_path.read_text()
for field, path in (('candidate_hash', out/('native/tenstorrent_pci_probe-native-q4-fpu-stream'+str(role))),
                    ('client_hash', out/('client/'+client_name))):
    expected = re.search(r'^'+field+r'=([0-9a-f]{64})$', parent, re.M); assert expected
    assert hashlib.sha256(path.read_bytes()).hexdigest() == expected.group(1), field
run('native-endpoints', [sys.executable, here/'run_pioneer_native_compact_q4_stream_native_tests.py',
                         out/'native', private, out/'endpoints', native_selector])
compiler = [os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            '-ffp-contract=off', '-fsanitize=address,undefined', *role_flags]
units = ['pioneer_q38_factored_q4_compare_test.cpp', 'pioneer_native_factored_q4_compare_lease_test.cpp',
         'pioneer_native_factored_q4_private_test.cpp']
for i, unit in enumerate(units):
    binary = out/('test'+str(i)); args = [*compiler]
    if i == 2: args += ['-I'+str(private), '-I'+str(out/'native')]
    args += [here/unit]
    if i == 2: args += [here.parent/'PioneerModelWeightCache.cpp']
    run('compile'+str(i), [*args, '-o', binary])
    run('test'+str(i), [binary, out/'native/image5120', *([original] if i == 2 else [])])
run('parent-gate', [sys.executable, here/'pioneer_native_factored_q4_parent_test.py', *client_selector])
if manual:
    run('manual-role', [sys.executable, here/'pioneer_native_factored_q4_manual_role_test.py',
                         *(['--configured'] if configured else [])])
if configured:
    run('old-manual-role', [sys.executable, here/'pioneer_native_factored_q4_manual_role_test.py'])
    run('launch-admission', [sys.executable, here/'pioneer_native_factored_q4_launch_admission_test.py', '--configured'])
run('old-clock-default', [sys.executable, here.parent/'build_q38_q4_kernel_timing.py', sdk, out/'old-clock', 5120])
same()
dependencies = {Path(__file__).resolve(), here/'pioneer_native_factored_q4_parent_test.py', parent_path,
                here.parent/'build_native_compact_q4_stream_diagnostic.py',
                here.parent/'build_native_factored_q4_compare_client.py',
                here.parent/'build_q38_factored_q4_compare.py',
                here/'run_pioneer_native_compact_q4_stream_native_tests.py'}
if manual: dependencies.add(here/'pioneer_native_factored_q4_manual_role_test.py')
if configured: dependencies.add(here/'pioneer_native_factored_q4_launch_admission_test.py')
def collect(command):
    data = subprocess.check_output(command, text=True)
    for name in data.replace('\\\n', '').split(':', 1)[1].split():
        path = Path(name).resolve()
        if path.is_relative_to(root): dependencies.add(path)
def compile_dependencies(argv):
    if '-c' not in argv: return
    j = argv.index('-o'); del argv[j:j+2]; argv[argv.index('-c')] = '-M'; collect(argv)
for line in (out/'native/build-argv.txt').read_text().splitlines():
    compile_dependencies(line.split('\t'))
# Producer headers must be included too, not only the generated driver image.
for argv in json.loads((out/'native/image5120/build-argv.json').read_text()):
    compile_dependencies(argv)
for i, unit in enumerate(units):
    collect([compiler[0], '-std=c++17', *role_flags, '-I'+str(private), '-I'+str(out/'native'), '-M', str(here/unit)])
dependencies.update(here.parent/('blackhole_q38_factored_q4_compare'+s) for s in ('.cpp', '_start.S', '.ld'))
dependencies.add(here.parent/'blackhole_q38_q4_kernel_clock.cpp')
for path in dependencies: assert path in frozen, path
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(dependencies)))
(out/'test-argv.json').write_text(json.dumps(commands, indent=2)+'\n')
(out/'private-source-pins.json').write_text(json.dumps(private_pins, sort_keys=True, indent=2)+'\n')
print('PASS_COMPARE'+str(role)+'_COMPLETE_LOCAL children'+str(len(exits))+'_stderr0_consumedSources'+str(len(dependencies))+
      '_NO_NATIVE_HARDWARE_EXECUTION_OR_TPS', flush=True)
