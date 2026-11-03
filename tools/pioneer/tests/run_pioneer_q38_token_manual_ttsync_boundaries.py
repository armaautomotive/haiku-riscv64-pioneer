#!/usr/bin/env python3
"""Whole new DATA,corruption and real-compiler PRIVATE role checks;NO card IO.

Native macro inputs come from preprocessing the ACTUAL private subject,
not guessed test-only substitutes. These are build perimeters,not native
lifecycle/watchdog,original-model,physical performance or Work acceptance.
"""
import ast
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require

require(len(sys.argv) in (4, 5), 'ROOT MANUAL_WORKER ACTUAL_NATIVE_BUILD [--public]')
public = len(sys.argv) == 5
require(not public or sys.argv[4] == '--public', 'exact public boundary switch')
require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role overrides')
here = pathlib.Path(__file__).resolve().parent
root, worker, private = map(pathlib.Path, sys.argv[1:4])
require(all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root, worker, private)), 'absolute directories')
generator = here.parent / 'generate_qwen38_token_programs.py'
helpers = runpy.run_path(str(generator))
inputs = ((root, helpers['pins'](here.parent / 'build_qwen38_token_phase_profile_root.py', 'PINS')),
          (worker, helpers['pins'](here.parent / 'build_qwen38_manual_ttsync_worker.py', 'PINS')))
for directory, records in inputs:
    helpers['authenticated'](directory, records)
header = here.parent / 'PioneerQ38TokenManualTTSyncPrograms.h'
require(hashlib.sha256(header.read_bytes()).hexdigest() == 'eaaa4974db72e416f13b2bdd66a4eb5bcdf4d4243a27c990037eea022e009dd1', 'whole frozen DATA header')
primary = 'PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS'
context = 'PIONEER_Q38_TOKEN_MANUAL_TTSYNC_' + ('BENCHMARK_NATIVE' if public else 'NATIVE_CONTEXT')
parent_context = 'PIONEER_Q38_TOKEN_CONSTANT_Q4_' + ('BENCHMARK_NATIVE' if public else 'NATIVE_CONTEXT')
build_script = here.parent / ('build_qwen38_token_manual_ttsync_benchmark_native.py' if public else 'build_qwen38_token_manual_ttsync_native.py')
build_key = 'OBJECT_SHA' if public else 'PRIVATE_SHA'
digests = [ast.literal_eval(n.value) for n in ast.parse(build_script.read_text()).body
           if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == build_key for t in n.targets)]
require(len(digests) == 1 and len(digests[0]) == 64, 'frozen native object pin')
object_name = 'token-manual-ttsync-benchmark-native.o' if public else 'token-manual-ttsync-NODRV.o'
require(hashlib.sha256((private / object_name).read_bytes()).hexdigest() == digests[0], 'whole actual native object')
role = here.parent / 'PioneerQ38TokenManualTTSyncProgramsRole.h'
selected = here.parent / 'PioneerQ38TokenSelectedPrograms.h'
include = lambda p: '#include "' + str(p) + '"\n'
with tempfile.TemporaryDirectory(prefix='pioneer-q38-manual-native-boundaries-', dir='/private/tmp') as temporary:
    out = pathlib.Path(temporary)
    generated = out / 'manual-data.h'
    subprocess.run([sys.executable, str(generator), str(root), str(worker), str(generated), '--manual-ttsync'], check=True)
    require(generated.read_bytes() == header.read_bytes(), 'exact regenerated complete header')
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I' + str(here.parent)]
    binary = out / 'data-test'
    subprocess.run([*common, '-O1', '-g', '-fsanitize=address,undefined', '-D' + primary + '=1',
                    '-DPIONEER_Q38_PROGRAM_DATA_TEST_MANUAL_TTSYNC=1',
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="' + str(generated) + '"',
                    str(here / 'pioneer_q38_pushed_programs_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root), str(worker)], check=True)
    corrupted = 0
    for directory, records in inputs:
        for name, _, _ in records:
            case = out / ('bad-' + str(corrupted))
            case.mkdir()
            for file, _, _ in records:
                (case / file).symlink_to(directory / file)
            target = case / name
            target.unlink()  # exact NEW local fixture link only
            original = (directory / name).read_bytes()
            changed = bytearray(original)
            changed[len(changed) // 2] ^= 1
            for raw in (changed, original[:-1]):
                target.write_bytes(raw)
                output = out / ('NO_OUTPUT-' + str(corrupted))
                result = subprocess.run([sys.executable, str(generator), str(case if directory == root else root),
                                         str(case if directory == worker else worker), str(output), '--manual-ttsync'], capture_output=True)
                require(result.returncode == 2 and name.encode() in result.stderr and not output.exists(), 'actual corruption/truncation refusal ' + name)
                corrupted += 1
    require(corrupted == 32, 'all16 complete sections/ELFs corrupted AND truncated')
    wrong = 0
    for r, w, mode in ((root, worker / 'unchanged-constant-q4', '--manual-ttsync'),
                       (root, worker, '--constant-q4-issue'), (root, worker, '--token-matrix-phase')):
        output = out / ('WRONG-' + str(wrong))
        result = subprocess.run([sys.executable, str(generator), str(r), str(w), str(output), mode], capture_output=True)
        require(result.returncode == 2 and not output.exists(), 'actual wrong-family refusal')
        wrong += 1
    # Preprocess exact selected PRIVATE compile unit using its own emitted
    # compiler argv. NEVER execute a caller shell or overwrite the object.
    command = (private / 'compile-argv.txt').read_text().splitlines()
    require(command[0] == '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++', 'known cross compiler')
    source_name = 'pioneer_q38_token_manual_ttsync_benchmark_native_compile.cpp' if public else 'pioneer_q38_token_manual_ttsync_native_context_compile.cpp'
    require(command[command.index('-c') + 1] == str(here / source_name), 'actual own native source')
    command[command.index('-c')] = '-E'
    command[command.index('-o') + 1] = str(out / 'actual-native-macros.txt')
    subprocess.run([*command, '-dM'], check=True)
    base = {}
    for row in (out / 'actual-native-macros.txt').read_text().splitlines():
        match = re.fullmatch(r'#define ([A-Za-z_][A-Za-z0-9_]*)(?: (.*))?', row)
        if not match:
            continue
        name, value = match[1], match[2] or ''
        if ((name.startswith(('PIONEER_', 'TT_PIONEER_')) and not name.endswith('_H'))
                or name == '_KERNEL_MODE' or name in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')):
            base[name] = value
    require(base.get(primary) == base.get(context) == '1' and base.get('_KERNEL_MODE') == '1', 'actual selected native macros')
    positive = negative = 0

    def syntax(values, okay, body=None):
        global positive, negative
        source = ''.join('#define ' + key + ' ' + value + '\n' for key, value in sorted(values.items()))
        source += body if body is not None else include(role) + include(role)
        result = subprocess.run([*common, '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, capture_output=True)
        require((result.returncode == 0) == okay, 'production role decision ' + repr((values, body, result.stderr)))
        if okay:
            positive += 1
        else:
            require('error:' in result.stderr, 'actual compiler refusal')
            negative += 1

    syntax({}, True)
    syntax({primary: '1'}, True, include(header) + include(header))
    syntax(base, True)
    syntax(base, True, include(selected) + include(selected)
           + 'static_assert(PioneerQ38TokenManualTTSyncDataSelection==' + str(3 if public else 2) + ' && PioneerQ38TokenPrograms::ManualTTSync==1);\n')
    old = {k: v for k, v in base.items() if k not in (primary, context)}
    syntax(old, True, include(selected) + 'static_assert(PioneerQ38TokenManualTTSyncDataSelection==0 && PioneerQ38TokenPrograms::ConstantQ4Issue==1);\n')
    for name in (primary, context, 'PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS', parent_context):
        for bad in (None, '', '0', '2'):
            value = dict(base)
            if bad is None:
                del value[name]
            else:
                value[name] = bad
            syntax(value, False)
        syntax(base, False, include(selected) + '#undef ' + name + '\n' + include(selected))
    forbidden = ('PIONEER_Q38_TOKEN_MANUAL_TTSYNC_CLIENT', 'PIONEER_Q38_MANUAL_TTSYNC_IMAGE', 'PIONEER_Q38_MANUAL_TTSYNC_PARENT_BINDING',
                 'PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS', 'PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT',
                 'PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT', 'PIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT',
                 'PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS_H', 'PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS_H',
                 'PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS_H', 'PIONEER_Q38_CPU_VALIDATION',
                 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE',
                 '__FAST_MATH__', 'PIONEER_Q38_TOKEN_MANUAL_TTSYNC_' + ('NATIVE_CONTEXT' if public else 'BENCHMARK_NATIVE'))
    for name in forbidden:
        for bad in ('0', '1'):
            syntax({**base, name: bad}, False)
            syntax(base, False, include(role) + '#define ' + name + ' ' + bad + '\n' + include(role))
    for name in (primary, context):
        syntax(old, False, include(selected) + '#define ' + name + ' 1\n' + include(selected))
    for name in ('PioneerQ38TokenConstantQ4Programs.h', 'PioneerQ38TokenMatrixPhasePrograms.h',
                 'PioneerQ38TokenPhaseProfilePrograms.h', 'PioneerQ38TokenWideGdnPrograms.h'):
        syntax(base, False, include(here.parent / name) + include(selected))
        syntax(base, False, include(selected) + include(here.parent / name))
    print(f'PASS_MANUAL_TTSYNC_DATA_{"PUBLIC" if public else "PRIVATE"}_BOUNDARIES all120codeWords3109912 '
          f'corruptTruncated{corrupted} wrongFamilies{wrong} syntaxPositives{positive} productionRefusals{negative};'
          'actual compiler macros/old constant-Q4 positive/unchanged journal;'
          'NOT native lifecycle,watchdog,original-model,hardware,Work or TPS qualification', flush=True)
