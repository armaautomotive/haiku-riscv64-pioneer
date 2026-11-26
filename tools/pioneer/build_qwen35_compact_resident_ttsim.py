#!/usr/bin/env python3
"""Build a non-packageable two-region Qwen3.5 resident witness.

The image uses a size-optimized toolchain build and omits the extra 0xd0000
code window. The optional profiled variant is compile-only. Neither variant
grants hardware ownership or recovery authority.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, help='configured Haiku RISC-V build directory')
    parser.add_argument('output', type=Path, help='new absolute output directory')
    parser.add_argument('--mixed-optimization', action='store_true',
                        help='compile the stream and graph dispatch objects with -O2')
    parser.add_argument('--profiled-compile-only', action='store_true',
                        help='compile normal cycle-counter profiling; do not execute in ttsim')
    parser.add_argument('--qualification', action='store_true',
                        help='build the matching standalone qualification service, compile-only')
    parser.add_argument('--ttsim-qualification', action='store_true',
                        help='build a separately marked unprofiled standalone service for ttsim only')
    args = parser.parse_args()
    if args.ttsim_qualification and (not args.qualification or args.profiled_compile_only):
        parser.error('ttsim qualification requires an unprofiled qualification build')
    if args.qualification and not (args.profiled_compile_only or args.ttsim_qualification):
        parser.error('qualification is restricted to the normal-counter compile-only image unless explicitly simulator-only')
    if not args.output.is_absolute() or args.output.exists() or args.output.is_symlink():
        parser.error('output must be a new absolute directory')
    root = Path(__file__).resolve().parent
    build = args.build.resolve(strict=True)
    cross = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku'
    sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    if not (sdk / 'headers').is_dir():
        parser.error('Haiku development headers not found')
    args.output.mkdir(mode=0o700)
    for marker in ('activation-sharing', 'delta-shard', 'ffn-pair', 'flash-hybrid',
                   'fused-rms-save', 'compact-two-region'):
        (args.output / (marker + '.marker')).touch(exist_ok=False)
    (args.output / ('profiled-compile-only.marker' if args.profiled_compile_only
                    else 'ttsim-unprofiled.marker')).touch(exist_ok=False)
    if args.mixed_optimization:
        (args.output / 'compact-mixed-optimization.marker').touch(exist_ok=False)
    if args.qualification:
        (args.output / 'compact-qualification.marker').touch(exist_ok=False)
    if args.ttsim_qualification:
        (args.output / 'ttsim-qualification.marker').touch(exist_ok=False)

    flags = ('-std=c++17 -Oz -Wall -Wextra -Werror -ffreestanding -fno-exceptions '
             '-fno-rtti -fno-stack-protector -fno-pic -fno-pie -fno-builtin '
             '-ffp-contract=off -ffunction-sections -fdata-sections -fno-unwind-tables '
             '-fno-asynchronous-unwind-tables -fstack-usage -msmall-data-limit=0 '
             '-mno-relax -march=rv32imf_zicsr -mabi=ilp32').split()
    defines = ('PIONEER_QWEN35_DEVICE_DISPATCH PIONEER_QWEN35_RECOVERY_COMPILE_ONLY '
               'PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS PIONEER_QWEN35_PAIRED_RECURRENCE '
               'PIONEER_QWEN35_FUSED_RECURRENCE PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS '
               'PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY '
               'PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY').split()
    if not args.profiled_compile_only:
        defines.append('PIONEER_QWEN35_TTSIM_UNPROFILED_COMPILE_ONLY')
    includes = ['-I' + str(root), '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix')]
    files = [('service.o', 'PioneerQwen35DataflowServiceImage.cpp'),
             ('device-stream.o', 'tests/pioneer_qwen35_dataflow_device_stream_target_probe.cpp'),
             ('device-barrier.o', 'tests/pioneer_qwen35_dataflow_device_barrier_target_probe.cpp'),
             ('device-graph.o', 'PioneerQwen35DataflowDeviceDispatch.cpp'),
             ('shared-roles.o', 'PioneerQwen35DataflowSharedRoles.cpp')]
    if not args.qualification:
        files.append(('resident-service-entry.o', 'PioneerQwen35DataflowResidentServiceEntry.cpp'))
    for name, source in files:
        extra = (['-DPIONEER_QWEN35_TREE_SETTLEMENT', '-fno-inline-functions-called-once']
                 if name == 'resident-service-entry.o' else [])
        if args.mixed_optimization and name in ('device-stream.o', 'device-graph.o'):
            extra.append('-O2')
        subprocess.run([str(cross) + '-g++', *flags, *['-D' + item for item in defines],
                        *extra, *includes, '-c', str(root / source), '-o', str(args.output / name)], check=True)
    subprocess.run([str(cross) + '-g++', *flags, *includes, '-c',
                    str(root / 'PioneerQwen35DataflowMemory.cpp'), '-o', str(args.output / 'memory.o')], check=True)
    start = ('blackhole_qwen35_dataflow_service_start.S' if args.qualification
             else 'blackhole_qwen35_resident_service_start.S')
    start_object = 'qualification-start.o' if args.qualification else 'resident-start.o'
    subprocess.run([str(cross) + '-as', '-march=rv32imf_zicsr', '-mabi=ilp32', '-mno-relax',
                    str(root / start), '-o', str(args.output / start_object)], check=True)
    objects = [start_object, 'service.o', 'memory.o', 'device-stream.o', 'device-barrier.o',
               'device-graph.o', 'shared-roles.o']
    if not args.qualification:
        objects.append('resident-service-entry.o')
    elf = args.output / 'service.elf'
    subprocess.run([str(cross) + '-ld', '-m', 'elf32lriscv', '--no-relax', '--gc-sections',
                    '-u', 'PioneerQwen35DeviceGraphRun', '-T',
                    str(root / 'blackhole_qwen35_compact_resident_ttsim.ld'),
                    *[str(args.output / name) for name in objects], '-Map', str(args.output / 'service.map'),
                    '-o', str(elf)], check=True)
    undefined = subprocess.check_output([str(cross) + '-nm', '-u', str(elf)], text=True)
    if undefined:
        raise RuntimeError('compact simulator image has unresolved imports')
    symbols = subprocess.check_output([str(cross) + '-nm', '-n', str(elf)], text=True)
    (args.output / 'symbols.txt').write_text(symbols)
    disassembly = subprocess.check_output([str(cross) + '-objdump', '-d', str(elf)], text=True)
    (args.output / 'disassembly.txt').write_text(disassembly)
    if re.search(r'\s(?:amo[a-z]+\.w|lr\.w|sc\.w|(?:fdiv|fsqrt)\.[sd]|fcvt\.(?:w|wu)\.[sd])\s', disassembly):
        raise RuntimeError('compact image uses unsupported RV32 operations')
    main_name = 'qwen35_dataflow_service_main' if args.qualification else 'qwen35_resident_graph_service_main'
    for name in (main_name, 'qwen35_graph_run_descriptor',
                 'qwen35_shared_compute_coordinate', 'TraceProjection', 'MatrixPanel'):
        if not re.search(r'jalr[^\n]*' + re.escape(name), disassembly):
            raise RuntimeError('missing call in compact image: ' + name)
    images = {}
    for section in ('boot', 'text', 'dispatch'):
        target = args.output / (section + '.bin')
        subprocess.run([str(cross) + '-objcopy', '-O', 'binary', '-j', '.' + section,
                        str(elf), str(target)], check=True)
        data = target.read_bytes()
        images[section] = data
        print(f'{section}: {len(data)} bytes sha256 {hashlib.sha256(data).hexdigest()}', flush=True)
    if images['boot'] != bytes.fromhex('6fc00600'):
        raise RuntimeError('unexpected compact boot jump')
    if (not 0 < len(images['text']) <= 0xe000 or len(images['text']) % 4
            or not 0 < len(images['dispatch']) <= 0x9c00 or len(images['dispatch']) % 4):
        raise RuntimeError('compact image exceeds the declared saved ranges')
    addresses = {}
    for row in symbols.splitlines():
        parts = row.split()
        if len(parts) != 3 or parts[1] not in ('T', 't', 'W', 'w'):
            continue
        address, _, name = parts
        address = int(address, 16)
        if not (0x6000 <= address < 0x6004
                or 0x72000 <= address < 0x72000 + len(images['text'])
                or 0xa0400 <= address < 0xa0400 + len(images['dispatch'])):
            raise RuntimeError('compact code symbol outside the two ranges: ' + name)
        addresses[name] = address
    if addresses.get('qwen35_dataflow_entry') != 0x72000:
        raise RuntimeError('unexpected compact service entry')
    park_name = 'qwen35_dataflow_stopped_park' if args.qualification else 'qwen35_resident_graph_stopped_park'
    park = addresses.get(park_name, 0) - 0x72000
    if park < 0 or park % 4 or images['text'][park:park + 4] != bytes.fromhex('6f000000'):
        raise RuntimeError('unexpected compact stopped-park instruction')

    # Reuse the existing resident stack checks without producing a driver header.
    source = (root / ('build_qwen35_dataflow_service_image.sh' if args.qualification
                      else 'build_qwen35_resident_service_witness.sh')).read_text()
    needle = ("python3 - \"$output\" \"$role_graph\" <<'PY'\n" if args.qualification else "<<'PY'\n")
    if source.count(needle) != 1:
        raise RuntimeError('resident stack-check script changed')
    start = source.index(needle) + len(needle)
    end = source.index('\nPY\n', start)
    stack_args = ([str(args.output), '1'] if args.qualification else
                  [str(args.output), str(args.output), '1', '1', '0', '1', '1'])
    subprocess.run([sys.executable, '-B', '-c', source[start:end], *stack_args], check=True)
    print('NON-LOADABLE WITNESS; no driver, physical ownership, recovery or throughput qualification', flush=True)


if __name__ == '__main__':
    main()
