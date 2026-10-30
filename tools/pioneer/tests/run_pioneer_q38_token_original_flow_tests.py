#!/usr/bin/env python3
"""Actual pinned original27B whole-token instructions, NEVER hardware or TPS."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
options = sys.argv[5:]
observed = None
if '--observed-trace' in options:
    at = options.index('--observed-trace')
    assert at+1 < len(options)
    observed = pathlib.Path(options[at+1])
    options = options[:at]+options[at+2:]
    assert observed.is_absolute() and not observed.exists() and not observed.is_symlink()
assert 5 <= len(sys.argv) <= 23 and len(set(options)) == len(options) and set(options) <= {'--prepare-only', '--bootstrap-only', '--first-layer-only', '--fast-root-memory', '--cancel-boundary', '--unroll-dense-seals', '--boundary-seals', '--unroll-quiet', '--worker-local-source-guards', '--precise-activation', '--single-closed-audit', '--worker-sealed-admission', '--snapshot-first-layer', '--eos-after-prompt', '--admission-source-faults', '--pushed-completion', '--pushed-completion-faults', '--pushed-cohort-faults', '--pushed-admission-faults', '--pushed-dense-faults'}
unroll = '--unroll-dense-seals' in options
boundary = '--boundary-seals' in options
assert not unroll or '--cancel-boundary' in options,'dense-unroll candidate is an explicitly cancellable FP32 image'
assert not boundary or (unroll and '--cancel-boundary' in options),'boundary candidate requires explicit cancellable unrolled FP32 image'
quiet = '--unroll-quiet' in options
assert not quiet or (boundary and unroll and '--cancel-boundary' in options),'queue-check candidate requires explicit boundary+cancellable+unrolled FP32 image'
local_sources = '--worker-local-source-guards' in options
assert not local_sources or quiet,'worker-local source candidate requires the explicit queue/boundary/cancellable FP32 image'
precise = '--precise-activation' in options
assert not precise or local_sources,'precise activation requires the distinct FP32/local-source ROOT and QGW2 worker images'
single_audit = '--single-closed-audit' in options
assert not single_audit or precise,'single closed audit requires the explicit QGW2 precise profile'
worker_admission = '--worker-sealed-admission' in options
assert not worker_admission or single_audit,'worker-sealed admission requires the explicit QGW2 single-audit profile'
pushed='--pushed-completion' in options
assert not pushed or worker_admission,'QGW3 receiver requires the entire explicit worker-admission profile'
pushed_faults='--pushed-completion-faults' in options
pushed_cohort='--pushed-cohort-faults' in options
pushed_admission='--pushed-admission-faults' in options
pushed_dense='--pushed-dense-faults' in options
assert not pushed_dense or (pushed and observed is None and not any(o in options for o in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory','--snapshot-first-layer','--eos-after-prompt','--admission-source-faults','--pushed-completion-faults','--pushed-cohort-faults','--pushed-admission-faults'))), 'QGW3 large dense faults require exact original bytes and no shortcuts,other fault modes or trace publication'
assert not pushed_faults or (pushed and observed is None and not any(o in options for o in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory','--snapshot-first-layer','--eos-after-prompt','--admission-source-faults','--pushed-cohort-faults','--pushed-admission-faults'))), 'QGW3 connected faults require an unshortened exact original-byte profile without trace publication'
assert not pushed_cohort or (pushed and (observed is None or '--snapshot-first-layer' in options) and not any(o in options for o in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory','--eos-after-prompt','--admission-source-faults','--pushed-completion-faults','--pushed-admission-faults'))), 'QGW3 cohort faults require exact original bytes;observations require full continuing snapshot mode,not a shortcut'
assert not pushed_admission or (pushed and observed is None and not any(o in options for o in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory','--snapshot-first-layer','--eos-after-prompt','--admission-source-faults','--pushed-completion-faults','--pushed-cohort-faults'))), 'QGW3 retained-admission faults require exact original bytes and no shortcuts or trace publication'
admission_faults = '--admission-source-faults' in options
assert not admission_faults or (single_audit and observed is None and not any(o in options for o in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory','--snapshot-first-layer','--eos-after-prompt'))), 'connected admission fault test requires exact single-audit/worker-admission original-byte profile without shortcuts or trace publication'
assert sum(o in options for o in ('--prepare-only', '--bootstrap-only', '--first-layer-only')) <= 1
assert observed is None or '--prepare-only' not in options
assert '--first-layer-only' not in options or observed is not None
eos = '--eos-after-prompt' in options
snapshot = '--snapshot-first-layer' in options
assert not snapshot or (observed is not None and not eos and not any(o in options for o in ('--prepare-only','--bootstrap-only','--first-layer-only')))
assert not eos or '--first-layer-only' not in options
assert not eos or observed is not None or '--prepare-only' in options
eos_state = pathlib.Path(str(observed)+'-state') if eos and observed is not None else None
if eos_state is not None:
    assert not eos_state.exists() and not eos_state.is_symlink(), 'fresh separate EOS state directory required'
image, old, worker, model = map(pathlib.Path, sys.argv[1:5])
assert all(p.is_absolute() for p in (image, old, worker, model))
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
for directory, script, key in ((image, here.parent/'build_qwen38_token_root.py', 'PINS_CANCEL_F32_PUSHED' if pushed else 'PINS_CANCEL_F32_WORKER_ADMISSION' if worker_admission else 'PINS_CANCEL_F32_SINGLE_AUDIT' if single_audit else 'PINS_CANCEL_F32_PRECISE' if precise else 'PINS_CANCEL_F32_LOCAL_SOURCE' if local_sources else 'PINS_CANCEL_F32_QUIET' if quiet else 'PINS_CANCEL_F32_BOUNDARY' if boundary else 'PINS_CANCEL_F32_UNROLLED_DENSE' if unroll else 'PINS_CANCEL_F32' if '--cancel-boundary' in options else 'PINS_F32'),
                               (old, here/'run_pioneer_q38_mixed_worker_tests.py', 'PINS'),
                               (worker, here.parent/'build_qwen38_pushed_worker_image.py' if pushed else here/'run_pioneer_q38_precise_worker_tests.py' if precise else here.parent/'build_qwen38_global_worker_image.py', 'PINS')):
    tree = ast.parse(script.read_text())
    pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == key for t in n.targets))
    assert pins
    for name, size, digest in pins:
        data = (directory/name).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, (directory, name)
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
for directory in (image, worker):
    assert subprocess.check_output([tool+'nm', '-n', str(directory/'program.elf')], text=True) == (directory/'program.symbols').read_text()
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
if observed is not None:
    observed.mkdir(mode=0o700)
if eos_state is not None:
    eos_state.mkdir(mode=0o700)
with tempfile.TemporaryDirectory(prefix='pioneer-original-token-flow-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    recorder = []
    if observed is not None:
        bridge = pathlib.Path(tmp)/'eos-state-data.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
                        '-c', str(here/'pioneer_q38_eos_state_data_bridge.cpp'), '-o', str(bridge)], check=True)
        recorder = [str(bridge)]
    intervals = {}
    for line in subprocess.check_output([tool+'objdump', '-tC', str(image/'program.elf')], text=True).splitlines():
        p = line.split(None, 5)
        if len(p) != 6 or p[2] != 'F':
            continue
        start, size = int(p[0], 16), int(p[4], 16)
        if not size:
            continue
        assert not (start | size) & 3
        assert (0x40000 <= start and start+size <= 0x60000) or (0x64000 <= start and start+size <= 0x6c000)
        intervals.setdefault((start, size), p[5])
    end = 0
    for (start, size), name in sorted(intervals.items()):
        assert start >= end
        end = start+size
    profile = pathlib.Path(tmp)/'profile.map'
    profile.write_text(''.join(f'{start:x} {size:x} {name}\n' for (start, size), name in sorted(intervals.items())))
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off', '-fsanitize=address,undefined',
                    *flags, *(['-DPIONEER_Q38_TEST_FAST_ROOT_MEMORY=1'] if '--fast-root-memory' in options else []),
                    *(['-DPIONEER_Q38_TEST_CANCELLABLE_ROOT=1'] if '--cancel-boundary' in options else []),
                    *(['-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1'] if precise else []),
                    *(['-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1'] if pushed else []),
                    *(['-DPIONEER_Q38_TEST_OBSERVED_TRACE=1'] if observed is not None else []),
                    str(here/('pioneer_q38_connected_pushed_dense_fault_test.cpp' if pushed_dense else 'pioneer_q38_connected_pushed_admission_test.cpp' if pushed_admission else 'pioneer_q38_connected_pushed_cohort_test.cpp' if pushed_cohort else 'pioneer_q38_connected_pushed_completion_test.cpp' if pushed_faults else 'pioneer_q38_connected_admission_fault_test.cpp' if admission_faults else 'pioneer_q38_token_original_flow_test.cpp')), str(here/'pioneer_q38_whole_original_bytes.cpp'), *recorder, '-o', str(binary)], check=True)
    subprocess.run([str(binary), *sys.argv[1:5], *(['--expect-worker-sealed-admission'] if admission_faults and worker_admission else []), *[o for o in options if o not in ('--fast-root-memory', '--cancel-boundary', '--unroll-dense-seals', '--boundary-seals', '--unroll-quiet', '--worker-local-source-guards', '--precise-activation', '--single-closed-audit','--worker-sealed-admission','--admission-source-faults','--pushed-completion','--pushed-completion-faults','--pushed-cohort-faults','--pushed-admission-faults','--pushed-dense-faults')],
                    *(['--observed-trace', str(observed)] if observed is not None else [])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1', 'PIONEER_TEST_ROOT_PROFILE_MAP':str(profile)})
    if observed is not None and '--bootstrap-only' in options:
        assert not list(observed.iterdir()), 'partial observations MUST NOT publish a complete token trace'
        assert eos_state is None or not list(eos_state.iterdir()), 'bootstrap MUST NOT publish EOS state data'
    if '--first-layer-only' in options:
        assert sorted(p.name for p in observed.iterdir()) == ['prefix-0.bin'], 'first-layer diagnostic MUST NOT publish complete token traces'
    if eos_state is not None and '--bootstrap-only' not in options:
        from compare_pioneer_q38_eos_state import corpus
        assert {p.name for p in eos_state.iterdir()} == set(corpus()) | {'eos-state-complete.bin'}, 'missing EOS retained-state data'
