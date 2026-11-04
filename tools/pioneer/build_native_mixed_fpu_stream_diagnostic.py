#!/usr/bin/env python3
"""BUILD ONLY diagnostic87. PRIVATE weights/headers/binaries MUST NOT be published."""
import argparse
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('private_export', type=pathlib.Path)
parser.add_argument('original', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
build, private, original, output = args.haiku_build, args.private_export, args.original, args.output
assert build.is_absolute() and build.is_dir() and output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
assert root not in output.parents and root not in private.parents
pins = {'PioneerNativeMixedFpuStreamData.h': 'ada7d5650402e67fac810973b0290e60d7c6561840b91b4c4f0cf83776151f54',
        'PioneerNativeMixedFpuStreamMetadata.h': '580b4e36ca6fd729a0b5e6340b33bf372ab1ef9d661f8c66ffecfb7e6f98385c'}


def private_unchanged():
    assert private.is_absolute() and not private.is_symlink() and private.stat().st_uid == os.geteuid()
    assert stat.S_IMODE(private.stat().st_mode) == 0o700
    for name, digest in pins.items():
        fd = os.open(private / name, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
        try:
            info = os.fstat(fd)
            assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid()
            assert stat.S_IMODE(info.st_mode) == 0o600 and info.st_size < 16 * 1024 * 1024
            data = os.pread(fd, info.st_size + 1, 0)
            assert len(data) == info.st_size and hashlib.sha256(data).hexdigest() == digest
            after = os.fstat(fd)
            assert (info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns) == (after.st_ino, after.st_size, after.st_mtime_ns, after.st_ctime_ns)
        finally:
            os.close(fd)


private_unchanged()
subprocess.run([sys.executable, str(here / 'tests/pioneer_native_mixed_fpu_stream_export_test.py'), str(private), str(original)], check=True)
output.mkdir(mode=0o700)
images = {}
image_pins = ((14, 5120, 7848, 'af5b7c2e7ea4d1fa28ba7e0b11f8122bc5168ce8ba1416be1ef5b1319135b867'),
              (14, 6144, 7848, 'f6ced20d6861124fa23134dfe5af9fa70d7eb8ba8cf679b0fa101af242c3934d'),
              (8, 5120, 6812, '8ab2edda60d0b59f09d3f8162a04a10d2eec6a3aefe9a48f69188d4979c2f66e'),
              (8, 6144, 6812, '5948d8b1aad1292d5c589449822f0d63c0dcf357c10c77916973d588568bf99d'))
for kind, width, size, digest in image_pins:
    directory = output / f'q{6 if kind == 14 else 8}-image{width}'
    subprocess.run([sys.executable, str(here / f'build_q38_q{6 if kind == 14 else 8}_fpu_stream.py'), str(build), str(directory), str(width), '--size'], check=True)
    code, boot = (directory / 'text.bin').read_bytes(), (directory / 'boot.bin').read_bytes()
    assert len(code) == size and hashlib.sha256(code).hexdigest() == digest
    assert len(boot) == 4 and hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
    images[kind, width] = code
widths = [6144] * 3 + [5120] * 6 + [6144] * 3 + [5120] * 5
types = [14] * 6 + [8] * 11
header = ('#include <stdint.h>\nstruct PioneerNativeMixedFpuStreamGeneratedImage {\n'
          ' static constexpr unsigned Samples=17,Rows=32,Phases=4,LayoutRole=87,Formats=2,MaxBytes=8192;\n'
          ' static constexpr uint32_t Boot=0x%08x;\n' % int.from_bytes(boot, 'little'))
for name, values in [('Width', widths), ('Type', types), ('Bytes', [7848] * 6 + [6812] * 11)]:
    header += ' static unsigned %s(unsigned s){static const unsigned t[17]={%s};return s<17?t[s]:0;}\n' % (name, ','.join(map(str, values)))
header += ' static uint32_t Word(unsigned s,unsigned i){if(s>=17 || i>=Bytes(s)/4)return 0;\n'
for (kind, width), code in images.items():
    words = [int.from_bytes(code[i:i + 4], 'little') for i in range(0, len(code), 4)]
    header += ' static const uint32_t code%d_%d[%d]={\n' % (kind, width, len(words))
    header += ',\n'.join(','.join('0x%08x' % w for w in words[i:i + 8]) for i in range(0, len(words), 8)) + '\n };\n'
    header += ' if(Type(s)==%d && Width(s)==%d)return code%d_%d[i];\n' % (kind, width, kind, width)
header += ' return 0;}\n};\n'
(output / 'PioneerNativeMixedFpuStreamGeneratedImage.h').write_text(header)
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx = tool + 'g++'
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here), '-I' + str(private)]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
common = [cxx, '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti', '-fstack-usage',
          '-ffreestanding', '-finline', '-fno-semantic-interposition', '-fno-tree-vectorize', '-fno-use-cxa-atexit',
          '-fno-strict-aliasing', '-fno-delete-null-pointer-checks', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE']
roles = ['BANKS', 'PANEL', 'AFFINE', 'BLOCK', 'STREAM', 'ROWS', 'COHORT', 'COHORT_SEALED', 'COHORT_PAYLOAD', 'DISPATCH', 'DISPATCH_VECTOR']
native = ['-DTT_PIONEER_EXPERIMENTAL_Q38_' + name for name in roles]
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010087ULL', '-DPIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC=1']
commands = []


def run(command):
    commands.append(command)
    subprocess.run(command, check=True, cwd=output)


driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_mixed_fpu_stream_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
cases = [('missing-native-role', ['-UPIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC']),
         ('wrong-native-role', ['-UPIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC', '-DPIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC=2']),
         ('missing-reload', ['-UTT_PIONEER_DRIVER_RELOAD']),
         ('old-tag', ['-UPIONEER_DRIVER_BUILD_TAG', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010086ULL']),
         ('fast-math', ['-ffast-math'])]
for macro in ('PIONEER_NATIVE_Q4_FPU_STREAM_DIAGNOSTIC', 'PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC',
              'PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64', 'PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64',
              'PIONEER_NATIVE_BFP_DIAGNOSTIC', 'PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC', 'PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC',
              'PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_Q38_WORK_MODE',
              'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    cases.append((macro, ['-D' + macro + '=1']))
for label, extra in cases:
    # Preprocessing stdout contains PRIVATE weights:NEVER retain/publish it.
    result = subprocess.run(common + native + extra + includes + ['-E', str(driver / 'pioneer_native_mixed_fpu_stream_driver.cpp')],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
    assert result.returncode and ('Build only the exact diagnostic87' in result.stderr or 'Diagnostic87 cannot alias' in result.stderr), label
(output / 'driver-role-refusals.txt').write_text('\n'.join('PASS ' + label for label, _ in cases) + '\n')
for label, name, source, old, new, diagnostic in (
        ('image', 'PioneerNativeMixedFpuStreamGeneratedImage.h', header, 'LayoutRole=87', 'LayoutRole=86', 'exact diagnostic87 Q6/Q8 images only'),
        ('data', 'PioneerNativeMixedFpuStreamData.h', (private / 'PioneerNativeMixedFpuStreamData.h').read_text(), 'Role=87', 'Role=86', 'exact compiled diagnostic87 data only')):
    wrong = output / ('wrong-' + label)
    wrong.mkdir(mode=0o700)
    assert source.count(old) == 1
    (wrong / name).write_text(source.replace(old, new))
    result = subprocess.run(common + native + ['-I' + str(wrong)] + includes + ['-fsyntax-only', str(driver / 'pioneer_native_mixed_fpu_stream_driver.cpp')],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
    assert result.returncode and diagnostic in result.stderr
(output / 'placement-compile-refusals.txt').write_text('PASS wrong-image-role actual full driver template instantiation\nPASS wrong-data-role actual full driver template instantiation\n')


def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()


for obj, name in [('native.o', 'tenstorrent_pci_probe-native-mixed-fpu-stream87'), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
binary = output / 'tenstorrent_pci_probe-native-mixed-fpu-stream87'
exports = subprocess.check_output([tool + 'nm', '-D', str(binary)], text=True)
(output / 'native-exports.txt').write_text(exports)
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= {line.split()[-1] for line in exports.splitlines() if line.split()}
legacy_name = 'PioneerNativeMixedFpuStreamLegacyFindDevice'
legacy = subprocess.check_output([tool + 'objdump', '-d', '--disassemble=_Z' + str(len(legacy_name)) + legacy_name + 'PKc', str(binary)], text=True)
(output / 'native-legacy-entry.disassembly').write_text(legacy)
instructions = []
for line in legacy.splitlines():
    match = re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$', line)
    if match:
        instructions.append((match[1], match[2].strip()))
assert instructions == [('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'), ('ld', 's0,8(sp)'), ('li', 'a0,0'), ('add', 'sp,sp,16'), ('ret', '')]
assert hashlib.sha256((output / 'tenstorrent_pci_probe-default43-control').read_bytes()).hexdigest() == '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
private_unchanged()
for name in ('tenstorrent_pci_probe-native-mixed-fpu-stream87', 'tenstorrent_pci_probe-default43-control'):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print(f'PASS BUILD_ONLY actual native87 driver/all methods LINKED,{len(cases)} role refusals,actual wrong-image/wrong-data refusals,legacy NULL/control unchanged;PRIVATE headers unchanged;NO stage/open/hardware/full-model/speed authority')
