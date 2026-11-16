#!/usr/bin/env python3
"""Build/test the offline static Q4 decoder schedule against actual RV32 images."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

if any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit('refusing Pioneer role overrides')
here = Path(__file__).resolve().parent
root = here.parents[2]
prefix = os.environ.get('RV32_PREFIX', 'riscv64-elf-')
tools = {name: shutil.which(prefix+name) for name in ('g++', 'as', 'ld', 'objcopy', 'nm')}
if any(value is None for value in tools.values()):
    raise SystemExit('requires an RV32 GNU toolchain; set RV32_PREFIX if needed')
host_cxx = shutil.which(os.environ.get('CXX', 'c++'))
if host_cxx is None:
    raise SystemExit('requires a host C++ compiler')
compile_flags = [
    '-U__UINT32_TYPE__', '-D__UINT32_TYPE__=unsigned',
    '-std=c++17', '-Os', '-Wall', '-Wextra', '-Werror', '-ffreestanding',
    '-fno-builtin', '-fno-tree-loop-distribute-patterns', '-fno-exceptions',
    '-fno-rtti', '-fno-stack-protector', '-fno-pic', '-fno-pie',
    '-ffunction-sections', '-fdata-sections', '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables', '-msmall-data-limit=0', '-mno-relax',
    '-march=rv32i', '-mabi=ilp32', '-ffixed-s11', '-fstack-usage',
]
variants = {
    'reference': (),
    'paired': ('-DPIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE=1', '-fomit-frame-pointer'),
    'static': ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-fomit-frame-pointer'),
    'static_helper': ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1',
                      '-DPIONEER_Q38_CACHED_FPU_STATIC_NIBBLE_HELPER_IMAGE=1', '-fomit-frame-pointer'),
    'half_static': ('-DPIONEER_Q38_CACHED_FPU_HALF_STATIC_ISSUE_IMAGE=1', '-fomit-frame-pointer'),
}
matrix_source = here.parent/'blackhole_q38_cached_fpu_matrix.cpp'

def run(argv, **kwargs):
    subprocess.run(list(map(str, argv)), check=True, **kwargs)

def build_image(directory, source, linker_script, role, extra=()):
    directory.mkdir(mode=0o700)
    obj = directory/'program.o'
    run([tools['g++'], '-D'+role+'=1', *extra, *compile_flags, '-c', source, '-o', obj])
    start = directory/'start.o'
    asm = here.parent/('blackhole_q38_bf16_producer_start.S' if role == 'PIONEER_Q38_BF16_PRODUCER_IMAGE'
                       else 'blackhole_q38_cached_fpu_matrix_start.S')
    run([tools['as'], '--march=rv32i', '--mabi=ilp32', '-mno-relax', asm, '-o', start])
    elf = directory/'program.elf'
    script = here.parent/linker_script
    run([tools['ld'], '-m', 'elf32lriscv', '--no-relax', '--gc-sections', '-T', script,
         start, obj, '-Map', directory/'program.map', '-o', elf])
    assert not subprocess.check_output([tools['nm'], '-u', elf], text=True).strip()
    for section in ('boot', 'text'):
        output = directory/(section+'.bin')
        run([tools['objcopy'], '-O', 'binary', '-j', '.'+section, elf, output])
        data = output.read_bytes()
        assert len(data) == 4 if section == 'boot' else 0 < len(data) <= 16384 and not len(data) % 4
    stack = (directory/'program.su').read_text().splitlines()
    frames = [int(line.split('\t')[1]) for line in stack]
    assert frames and max(frames) <= 4096
    return directory

with tempfile.TemporaryDirectory(prefix='q38-static-issue-', dir='/private/tmp') as tmp:
    base = Path(tmp)
    producer = build_image(base/'producer', here.parent/'blackhole_q38_bf16_producer.cpp',
                           'blackhole_q38_bf16_producer.ld', 'PIONEER_Q38_BF16_PRODUCER_IMAGE')
    images = {}
    for name, flags in variants.items():
        images[name] = build_image(base/name, matrix_source,
                                   'blackhole_q38_cached_fpu_matrix.ld',
                                   'PIONEER_Q38_CACHED_FPU_MATRIX_IMAGE', flags)

    refused = 0
    for flags in (
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=0',),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=2',),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-D_KERNEL_MODE=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_WORK_INFERENCE=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DTT_PIONEER_Q38_WORK_MODE=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_CPU_VALIDATION=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_FULL_MODEL_CPU_REFERENCE=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_NIBBLE_HELPER_IMAGE=1',),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_CACHED_FPU_STATIC_NIBBLE_HELPER_IMAGE=0'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_CACHED_FPU_STATIC_NIBBLE_HELPER_IMAGE=1',
         '-DPIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE=1'),
        ('-DPIONEER_Q38_CACHED_FPU_STATIC_ISSUE_IMAGE=1', '-DPIONEER_Q38_CACHED_FPU_HALF_STATIC_ISSUE_IMAGE=1',
         '-DPIONEER_Q38_CACHED_FPU_STATIC_NIBBLE_HELPER_IMAGE=1'),
    ):
        result = subprocess.run([tools['g++'], '-march=rv32i', '-mabi=ilp32',
                                 '-DPIONEER_Q38_CACHED_FPU_MATRIX_IMAGE=1', *flags,
                                 '-E', matrix_source], stdout=subprocess.DEVNULL,
                                stderr=subprocess.PIPE, text=True)
        assert result.returncode and 'error:' in result.stderr, flags
        refused += 1

    half_static_conflict = subprocess.run(
        [tools['g++'], '-march=rv32i', '-mabi=ilp32',
         '-DPIONEER_Q38_CACHED_FPU_MATRIX_IMAGE=1',
         '-DPIONEER_Q38_CACHED_FPU_HALF_STATIC_ISSUE_IMAGE=1',
         '-DPIONEER_Q38_CACHED_FPU_PAIR_ISSUE_IMAGE=1', '-E', matrix_source],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    assert half_static_conflict.returncode and 'error:' in half_static_conflict.stderr
    refused += 1

    test = base/'issue-variant-test'
    run([host_cxx, '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fsanitize=address,undefined',
         here/'pioneer_q38_cached_fpu_pair_issue_test.cpp', '-o', test])
    results = {}
    for reference, candidate in (('reference', 'paired'), ('paired', 'static'), ('paired', 'half_static'),
                                 ('paired', 'static_helper')):
        result = subprocess.run([test, producer, images[reference], images[candidate]],
                                check=True, text=True, stdout=subprocess.PIPE)
        lines = result.stdout.splitlines()
        assert len(lines) == 7 and lines[-1].startswith('PASS_CACHED_ROOT_L1_ISSUE_VARIANT ')
        records = []
        for line in lines[:6]:
            match = re.search(r'width(\d+) rows(\d+) worker(\d+).*referenceRV32(\d+) candidateRV32(\d+)', line)
            assert match and 'sameIssuedConfigStream' in line and 'OutputsExact' in line
            width, rows, worker, before, after = map(int, match.groups())
            assert after < before
            records.append((width, rows, worker, before, after))
        results[(reference, candidate)] = records
        print(result.stdout, end='')

    pair_to_static = results[('paired', 'static')]
    pair_to_half_static = results[('paired', 'half_static')]
    pair_to_static_helper = results[('paired', 'static_helper')]
    largest = next(row for row in pair_to_static if row[:3] == (17408, 256, 119))
    half_largest = next(row for row in pair_to_half_static if row[:3] == (17408, 256, 119))
    helper_largest = next(row for row in pair_to_static_helper if row[:3] == (17408, 256, 119))
    reduction = (largest[3]-largest[4])*100/largest[3]
    half_reduction = (half_largest[3]-half_largest[4])*100/half_largest[3]
    helper_reduction = (helper_largest[3]-helper_largest[4])*100/helper_largest[3]
    assert reduction > 10.0, reduction
    assert helper_reduction > 10.0, helper_reduction
    full_static_bytes = (images['static']/'text.bin').stat().st_size
    half_static_bytes = (images['half_static']/'text.bin').stat().st_size
    helper_static_bytes = (images['static_helper']/'text.bin').stat().st_size
    assert half_static_bytes < full_static_bytes, (half_static_bytes, full_static_bytes)
    print(f'PASS STATIC_ISSUE_RV32_SIM largestCaseInstructionReduction={reduction:.2f}% '
          f'halfStaticReduction={half_reduction:.2f}% nibbleHelperReduction={helper_reduction:.2f}% '
          f'halfStaticTextBytes={half_static_bytes} fullStaticTextBytes={full_static_bytes} '
          f'fullStaticNibbleHelperTextBytes={helper_static_bytes} '
          f'TensixStreamExact=1 outputs704Exact=1 roleRefusals={refused} text<=16384 stack<=4096;'
          'NO CARD/TPS/WORK/HARDWARE GRANT')
