#!/usr/bin/env python3
"""Build ONLY diagnostic85/native methods. PRIVATE weights/binaries MUST NOT be published."""
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
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
build, private, output = args.haiku_build, args.private_export, args.output
assert build.is_absolute() and build.is_dir() and output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
pins = {'PioneerNativeQ4FpuData.h': '8e4c2dfa81b436ffc79e6719c41a4090c4ae2501bf1a1f17f53f5759cd5c548b',
        'PioneerNativeQ4FpuMetadata.h': 'd075e5a9e313e221ed626c563d7dd861d50c9334ca96ca7f54255be6193787c9'}
def private_unchanged():
    assert private.is_absolute() and not private.is_symlink() and private.stat().st_uid == os.geteuid()
    assert stat.S_IMODE(private.stat().st_mode) == 0o700
    for name, digest in pins.items():
        info = (private / name).lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid()
        assert stat.S_IMODE(info.st_mode) == 0o600
        assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
private_unchanged()
subprocess.run([sys.executable, str(here / 'tests/pioneer_native_q4_fpu_export_test.py'), str(private)], check=True)
output.mkdir(mode=0o700)
subprocess.run([sys.executable, str(here / 'build_q38_q4_fpu_block.py'), str(build), str(output / 'image'), '--direct'], check=True)
code = (output / 'image/text.bin').read_bytes()
boot = (output / 'image/boot.bin').read_bytes()
assert len(code) == 7036 and hashlib.sha256(code).hexdigest() == '7e3efc36b1447a3880081caf899175a87378f894ea684d7623572625242e04d4'
assert len(boot) == 4 and hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
words = [int.from_bytes(code[i:i + 4], 'little') for i in range(0, len(code), 4)]
header = ('#include <stdint.h>\nstruct PioneerNativeQ4FpuGeneratedImage {\n'
          ' static constexpr unsigned Width=256,Rows=32,Phases=4,LayoutRole=85,Bytes=7036;\n'
          ' static constexpr uint32_t Boot=0x%08x;\n' % int.from_bytes(boot, 'little') +
          ' static uint32_t Word(unsigned i){static const uint32_t code[1759]={\n' +
          ',\n'.join(','.join('0x%08x' % w for w in words[i:i + 8]) for i in range(0, len(words), 8)) +
          '\n };return i<1759?code[i]:0;}\n};\n')
(output / 'PioneerNativeQ4FpuGeneratedImage.h').write_text(header)
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
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010085ULL', '-DPIONEER_NATIVE_Q4_FPU_DIAGNOSTIC=1']
commands = []
def run(command):
    commands.append(command)
    subprocess.run(command, check=True, cwd=output)
driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_q4_fpu_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
cases = [('missing-native-role', ['-UPIONEER_NATIVE_Q4_FPU_DIAGNOSTIC']),
         ('wrong-native-role', ['-UPIONEER_NATIVE_Q4_FPU_DIAGNOSTIC', '-DPIONEER_NATIVE_Q4_FPU_DIAGNOSTIC=2']),
         ('missing-reload', ['-UTT_PIONEER_DRIVER_RELOAD']),
         ('old-tag', ['-UPIONEER_DRIVER_BUILD_TAG', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010084ULL']),
         ('fast-math', ['-ffast-math'])]
for macro in ('PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64',
              'PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64', 'PIONEER_NATIVE_BFP_DIAGNOSTIC',
              'PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC', 'PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC',
              'PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_Q38_WORK_MODE',
              'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    cases.append((macro, ['-D' + macro + '=1']))
for label, extra in cases:
    # Preprocessing stdout contains PRIVATE weights:NEVER retain or publish it.
    result = subprocess.run(common + native + extra + includes + ['-E', str(driver / 'pioneer_native_q4_fpu_driver.cpp')],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
    assert result.returncode and ('Build only the exact diagnostic85' in result.stderr or 'Diagnostic85 cannot alias' in result.stderr), label
(output / 'driver-role-refusals.txt').write_text('\n'.join('PASS ' + label for label, _ in cases) + '\n')
wrong = output / 'wrong-image';wrong.mkdir(mode=0o700)
(wrong / 'PioneerNativeQ4FpuGeneratedImage.h').write_text(header.replace('LayoutRole=85', 'LayoutRole=84'))
result = subprocess.run(common + native + ['-I' + str(wrong)] + includes + ['-fsyntax-only', str(driver / 'pioneer_native_q4_fpu_driver.cpp')],
                        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
assert result.returncode and 'exact QFP1 image only' in result.stderr
wrong_data = output / 'wrong-data';wrong_data.mkdir(mode=0o700)
(wrong_data / 'PioneerNativeQ4FpuData.h').write_text('#include <stdint.h>\nstruct PioneerNativeQ4FpuData {static constexpr unsigned Version=1,Width=256,Rows=32,Samples=3,Role=84;static uint32_t Word(unsigned,unsigned){return 0;}};\n')
result = subprocess.run(common + native + ['-I' + str(wrong_data)] + includes + ['-fsyntax-only', str(driver / 'pioneer_native_q4_fpu_driver.cpp')],
                        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
assert result.returncode and 'exact compiled diagnostic data only' in result.stderr
(output / 'placement-compile-refusals.txt').write_text('PASS wrong-image-role actual full driver template instantiation\nPASS wrong-data-role actual full driver template instantiation\n')
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
for obj, name in [('native.o', 'tenstorrent_pci_probe-native-q4-fpu85'), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
binary = output / 'tenstorrent_pci_probe-native-q4-fpu85'
exports = subprocess.check_output([tool + 'nm', '-D', str(binary)], text=True)
(output / 'native-exports.txt').write_text(exports)
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= {line.split()[-1] for line in exports.splitlines() if line.split()}
legacy_name = 'PioneerNativeQ4FpuLegacyFindDevice'
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
for name in ('tenstorrent_pci_probe-native-q4-fpu85', 'tenstorrent_pci_probe-default43-control'):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print(f'PASS BUILD_ONLY actual native85 driver/all methods LINKED,{len(cases)} role refusals,actual wrong-image/wrong-data refusals,legacy NULL/control unchanged;PRIVATE headers unchanged;NO stage/open/hardware/full-model/speed authority')
