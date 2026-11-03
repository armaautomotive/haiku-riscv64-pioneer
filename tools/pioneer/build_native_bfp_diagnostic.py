#!/usr/bin/env python3
"""Build distinct fixed native-BFP diagnostic driver/client. NEVER installs or opens GPU."""
import argparse
import hashlib
import os
import pathlib
import re
import struct
import subprocess

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Refusing optimized Python: exact image/build guards must execute.')

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
build = args.haiku_build
output = args.output
output.mkdir(mode=0o700)
expected = {4: 'adcec3cf5a66a2f00b979746928e1f8525a1c2274f37d0f3e35d3b6468ee6ef3',
            8: 'c21f4692d340b0bab397a513a315b9884b898804022e35948b2d029a6a128b19'}
images = {}
for bits in (4, 8):
    folder = output / ('bfp' + str(bits))
    subprocess.run(['python3', str(here / 'build_q38_native_bfp_matvec.py'), str(build), str(folder), str(bits)], check=True)
    boot = (folder / 'boot.bin').read_bytes()
    text = (folder / 'text.bin').read_bytes()
    assert hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
    assert hashlib.sha256(text).hexdigest() == expected[bits]
    symbols = {}
    for line in (folder / 'program.symbols').read_text().splitlines():
        value, _, name = line.split()
        symbols[name] = int(value, 16)
    for name, value in {'native_bfp_entry': 0x70000, 'native_bfp_done': 0x7000c,
                        'native_bfp_park': 0x70038, 'native_bfp_fault': 0x70040,
                        'native_bfp_fault_park': 0x70058}.items():
        assert symbols[name] == value
    assert text[0x38:0x40] == struct.pack('<II', 0x6f, 0xffdff06f)
    images[bits] = struct.unpack('<' + 'I' * (len(text) // 4), text)
header = ['/* Generated from freshly compiled, exact-SHA RV32I images. */',
          '#ifndef PIONEER_NATIVE_BFP_GENERATED_IMAGE_H', '#define PIONEER_NATIVE_BFP_GENERATED_IMAGE_H',
          'struct PioneerNativeBfpGeneratedImage {',
          ' static constexpr unsigned Boot=0x%08x;' % struct.unpack('<I', boot)[0],
          ' static unsigned Word(unsigned bits,unsigned index) {']
for bits in (4, 8):
    words = images[bits]
    header.append('  static const unsigned code%d[]={' % bits)
    for start in range(0, len(words), 8):
        header.append('   ' + ','.join('0x%08x' % w for w in words[start:start+8]) + ',')
    header.append('  };')
header += ['  return bits==4?(index<sizeof(code4)/4?code4[index]:0):bits==8?(index<sizeof(code8)/4?code8[index]:0):0;',
           ' }', '};', '#endif']
(output / 'PioneerNativeBfpGeneratedImage.h').write_text('\n'.join(header) + '\n')
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
cxx = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output)]
user_includes = ['-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix')]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
        user_includes += ['-isystem', str(path)]
common = [cxx, '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti', '-fstack-usage',
          '-ffreestanding', '-finline', '-fno-semantic-interposition', '-fno-tree-vectorize', '-fno-use-cxa-atexit',
          '-fno-strict-aliasing', '-fno-delete-null-pointer-checks', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE']
roles = ['BANKS', 'PANEL', 'AFFINE', 'BLOCK', 'STREAM', 'ROWS', 'COHORT', 'COHORT_SEALED', 'COHORT_PAYLOAD', 'DISPATCH', 'DISPATCH_VECTOR']
native = ['-DTT_PIONEER_EXPERIMENTAL_Q38_' + name for name in roles]
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110300010078ULL', '-DPIONEER_NATIVE_BFP_DIAGNOSTIC=1']
commands = []
def run(command):
    commands.append(command)
    subprocess.run(command, check=True, cwd=output)

driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_bfp_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
for obj, name in [('native.o', 'tenstorrent_pci_probe-native-bfp78'), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
tool_prefix = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
exports = subprocess.check_output([tool_prefix + 'nm', '-D', str(output / 'tenstorrent_pci_probe-native-bfp78')], text=True)
(output / 'native-exports.txt').write_text(exports)
names = {line.split()[-1] for line in exports.splitlines() if line.split()}
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= names
legacy = subprocess.check_output([tool_prefix + 'objdump', '-d', '--disassemble=_Z32PioneerNativeBfpLegacyFindDevicePKc',
                                  str(output / 'tenstorrent_pci_probe-native-bfp78')], text=True)
(output / 'native-legacy-entry.disassembly').write_text(legacy)
instructions = []
for line in legacy.splitlines():
    match = re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$', line)
    if match:
        instructions.append((match[1], match[2].strip()))
assert instructions == [('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'),
                        ('ld', 's0,8(sp)'), ('li', 'a0,0'), ('add', 'sp,sp,16'), ('ret', '')]
libs = []
for path in sorted((build / 'build_packages').glob('gcc_syslibs-*/lib')):
    libs += ['-L' + str(path)]
run([cxx, '-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror', *user_includes,
     '-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot'), *libs,
     str(here / 'pioneer_native_bfp_diagnostic.cpp'), '-o', str(output / 'pioneer-native-bfp-diagnostic')])
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
for name in ('tenstorrent_pci_probe-native-bfp78', 'tenstorrent_pci_probe-default43-control', 'pioneer-native-bfp-diagnostic'):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print('BUILD_ONLY fixed native-BFP diagnostic;NO installation,driver reload,physical test,model quality or performance claim')
