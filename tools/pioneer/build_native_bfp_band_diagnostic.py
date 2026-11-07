#!/usr/bin/env python3
"""Build distinct fixed full-K BFP8 diagnostic driver. NEVER installs or opens GPU."""
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
group = parser.add_mutually_exclusive_group()
group.add_argument('--bfp4', action='store_true', help='Build separate BFP4 diagnostic118 only')
group.add_argument('--bfp4-timed', action='store_true', help='Build separate timed BFP4 diagnostic119 only')
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
build = args.haiku_build
output = args.output
output.mkdir(mode=0o700)
expected = {5120: 'f69c2803d854bbef87f37df7cb96536e1f5d940060ed98fec6cb3200d50a9ad5',
            6144: 'a6516a81cdb2e40b1850a0e51c3c2c2d706eaa2aa0207b29cbf5c2c3feb1a09e',
            17408: '902a07bf9cc67387d139bee4b18b3a454a1d90baa3b70d2c3f87e2b0f3939575'}
if args.bfp4 or args.bfp4_timed:
    expected = {5120: 'a60b38559aca3f728bb64c83ece2afc76c7a6412881ec3a904056853e467ca38',
                6144: '8b93889b11b7e949745a2e759a7f4c3aeeb31ca60a02167e88bff1c8be708cc7',
                17408: '68442a24c2b547ea7dbc7fbf3ff72988797a4761600b12d3ae74c9f2fc23a29d'}
if args.bfp4_timed:
    expected = {5120:'67f8dcf0756715f6ab6efb5d320adab2d7cb4d155092b55ec1b1dd9f20dec005',
                6144:'d4bdd2d32bee6b2e46d750e0c1622361b078a385c84b9d73002bc4060662825e',
                17408:'b2a2dbe7244b629be316ee00c0220d7827a019cd79aec9009c3ed89099f15928'}
image_name = 'PioneerNativeBfp4TimedGeneratedImage' if args.bfp4_timed else 'PioneerNativeBfp4BandGeneratedImage' if args.bfp4 else 'PioneerNativeBfpBandGeneratedImage'
driver_name = 'tenstorrent_pci_probe-native-bfp4-timed119' if args.bfp4_timed else 'tenstorrent_pci_probe-native-bfp4-band118' if args.bfp4 else 'tenstorrent_pci_probe-native-bfp-band79'
client_name = 'pioneer-native-bfp4-timed-diagnostic' if args.bfp4_timed else 'pioneer-native-bfp4-band-diagnostic' if args.bfp4 else 'pioneer-native-bfp-band-diagnostic'
phases = 2 if args.bfp4 or args.bfp4_timed else 4
images = {}
for bits in (5120, 6144, 17408):
    folder = output / ('k' + str(bits))
    command = ['python3', str(here / ('build_native_bfp4_band.py' if args.bfp4 or args.bfp4_timed else 'build_native_bfp_band.py')), str(build), str(folder), str(bits)]
    if args.bfp4_timed:
        command.append('--timed')
    elif not args.bfp4:
        command.append('4')
    subprocess.run(command, check=True)
    boot = (folder / 'boot.bin').read_bytes()
    text = (folder / 'text.bin').read_bytes()
    assert hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
    assert hashlib.sha256(text).hexdigest() == expected[bits]
    symbols = {}
    for line in (folder / 'program.symbols').read_text().splitlines():
        value, _, name = line.split()
        symbols[name] = int(value, 16)
    for name, value in {'native_bfp_band_entry': 0x70000, 'native_bfp_band_done': 0x7000c,
                        'native_bfp_band_park': 0x70038, 'native_bfp_band_fault': 0x70040,
                        'native_bfp_band_fault_park': 0x70058}.items():
        assert symbols[name] == value
    assert text[0x38:0x40] == struct.pack('<II', 0x6f, 0xffdff06f)
    images[bits] = struct.unpack('<' + 'I' * (len(text) // 4), text)
header = ['/* Fresh compiled,exact-SHA fixed synthetic full-K images. */',
          '#ifndef PIONEER_NATIVE_BFP_BAND_GENERATED_IMAGE_H', '#define PIONEER_NATIVE_BFP_BAND_GENERATED_IMAGE_H',
          'template<unsigned K> struct ' + image_name + ';']
for width, words in images.items():
    header += ['template<> struct %s<%d> {' % (image_name, width),
               ' static constexpr unsigned Width=%d,Phases=%d,Boot=0x%08x;' % (width, phases, struct.unpack('<I', boot)[0]),
               ' static unsigned Word(unsigned index) {', '  static const unsigned code[]={']
    for start in range(0, len(words), 8):
        header.append('   ' + ','.join('0x%08x' % w for w in words[start:start+8]) + ',')
    header += ['  };return index<sizeof(code)/4?code[index]:0;', ' }', '};']
header.append('#endif')
(output / (image_name + '.h')).write_text('\n'.join(header) + '\n')
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
cxx = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here)]
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
native += (['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010119ULL', '-DPIONEER_NATIVE_BFP4_TIMED_DIAGNOSTIC=1'] if args.bfp4_timed
           else ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010118ULL', '-DPIONEER_NATIVE_BFP4_BAND_DIAGNOSTIC=1'] if args.bfp4
           else ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010079ULL', '-DPIONEER_NATIVE_BFP_BAND_DIAGNOSTIC=1'])
commands = []
def run(command):
    commands.append(command)
    subprocess.run(command, check=True, cwd=output)

driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_bfp_band_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
for obj, name in [('native.o', driver_name), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
tool_prefix = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
exports = subprocess.check_output([tool_prefix + 'nm', '-D', str(output / driver_name)], text=True)
(output / 'native-exports.txt').write_text(exports)
names = {line.split()[-1] for line in exports.splitlines() if line.split()}
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= names
legacy = subprocess.check_output([tool_prefix + 'objdump', '-d', '--disassemble=_Z36PioneerNativeBfpBandLegacyFindDevicePKc',
                                  str(output / driver_name)], text=True)
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
run([cxx, '-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror', *(['-DPIONEER_NATIVE_BFP4_TIMED_CLIENT=1'] if args.bfp4_timed else ['-DPIONEER_NATIVE_BFP4_BAND_CLIENT=1'] if args.bfp4 else []), *user_includes,
     '-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot'), *libs,
     str(here / 'pioneer_native_bfp_band_diagnostic.cpp'), '-o', str(output / client_name)])
assert hashlib.sha256((output / 'tenstorrent_pci_probe-default43-control').read_bytes()).hexdigest() == '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
for name in (driver_name, 'tenstorrent_pci_probe-default43-control', client_name):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print('BUILD_ONLY separate full-K diagnostic', 'TIMED BFP4' if args.bfp4_timed else 'BFP4' if args.bfp4 else 'BFP8', ';NO installation,reload,physical test,model-quality or performance authority')
