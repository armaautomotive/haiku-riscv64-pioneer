#!/usr/bin/env python3
"""Build distinct fixed learned-weight diagnostic80;PRIVATE payload required. NEVER installs or opens GPU."""
import argparse
import hashlib
import os
import pathlib
import re
import struct
import stat
import subprocess

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Refusing optimized Python: exact image/build guards must execute.')

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('private_export', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
assert args.haiku_build.is_absolute() and args.haiku_build.is_dir()
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
build = args.haiku_build
output = args.output
private=args.private_export
assert private.is_absolute() and not private.is_symlink()
identity=private.stat()
assert stat.S_ISDIR(identity.st_mode) and identity.st_uid==os.geteuid() and stat.S_IMODE(identity.st_mode)==0o700
private_pins={
 'PioneerNativeBfpModelBandData.h':'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
 'PioneerNativeBfpModelBandMetadata.h':'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
for name,digest in private_pins.items():
 info=(private/name).lstat()
 assert stat.S_ISREG(info.st_mode) and info.st_nlink==1 and info.st_uid==os.geteuid() and stat.S_IMODE(info.st_mode)==0o600
 assert hashlib.sha256((private/name).read_bytes()).hexdigest()==digest
output.mkdir(mode=0o700)
expected = {5120:'f69c2803d854bbef87f37df7cb96536e1f5d940060ed98fec6cb3200d50a9ad5'}
images = {}
for bits in (5120,):
    folder = output / ('k' + str(bits))
    subprocess.run(['python3', str(here / 'build_native_bfp_band.py'), str(build), str(folder), str(bits), '4'], check=True)
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
header = ['/* Fresh compiled,exact-SHA full-K BFP8/all-four-phase images. */',
          '#ifndef PIONEER_NATIVE_BFP_BAND_GENERATED_IMAGE_H', '#define PIONEER_NATIVE_BFP_BAND_GENERATED_IMAGE_H',
          'template<unsigned K> struct PioneerNativeBfpBandGeneratedImage;']
for width, words in images.items():
    header += ['template<> struct PioneerNativeBfpBandGeneratedImage<%d> {' % width,
               ' static constexpr unsigned Width=%d,Phases=4,Boot=0x%08x;' % (width, struct.unpack('<I', boot)[0]),
               ' static unsigned Word(unsigned index) {', '  static const unsigned code[]={']
    for start in range(0, len(words), 8):
        header.append('   ' + ','.join('0x%08x' % w for w in words[start:start+8]) + ',')
    header += ['  };return index<sizeof(code)/4?code[index]:0;', ' }', '};']
header.append('#endif')
(output / 'PioneerNativeBfpBandGeneratedImage.h').write_text('\n'.join(header) + '\n')
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
cxx = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here), '-I' + str(private)]
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
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010080ULL', '-DPIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC=1']
commands = []
def run(command):
    commands.append(command)
    subprocess.run(command, check=True, cwd=output)

driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_model_band_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
for obj, name in [('native.o', 'tenstorrent_pci_probe-native-model-band80'), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
tool_prefix = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
exports = subprocess.check_output([tool_prefix + 'nm', '-D', str(output / 'tenstorrent_pci_probe-native-model-band80')], text=True)
(output / 'native-exports.txt').write_text(exports)
names = {line.split()[-1] for line in exports.splitlines() if line.split()}
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= names
legacy = subprocess.check_output([tool_prefix + 'objdump', '-d', '--disassemble=_Z38PioneerNativeModelBandLegacyFindDevicePKc',
                                  str(output / 'tenstorrent_pci_probe-native-model-band80')], text=True)
(output / 'native-legacy-entry.disassembly').write_text(legacy)
instructions = []
for line in legacy.splitlines():
    match = re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$', line)
    if match:
        instructions.append((match[1], match[2].strip()))
assert instructions == [('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'),
                        ('ld', 's0,8(sp)'), ('li', 'a0,0'), ('add', 'sp,sp,16'), ('ret', '')]
libs=[]
for path in sorted((build/'build_packages').glob('gcc_syslibs-*/lib')):
 libs+=['-L'+str(path)]
run([cxx,'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',*user_includes,'-I'+str(private),
     '-I'+str(root/'headers/private/shared'),'-I'+str(root/'headers/private/libroot'),
     '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'kits'),*libs,
     str(here/'pioneer_native_model_band_diagnostic.cpp'),str(here/'PioneerModelWeightCache.cpp'),
     str(release/'kits/shared/libshared.a'),'-lbe','-o',str(output/'pioneer-native-model-band-diagnostic')])
for name,digest in private_pins.items():
 assert hashlib.sha256((private/name).read_bytes()).hexdigest()==digest
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
for name in ('tenstorrent_pci_probe-native-model-band80','tenstorrent_pci_probe-default43-control','pioneer-native-model-band-diagnostic'):
 print(hashlib.sha256((output/name).read_bytes()).hexdigest(),name)
print('BUILD_ONLY learned-band driver/client;PRIVATE WEIGHT HEADERS NOT COPIED_OR_PUBLISHED;NO install/device/model-quality/speed authority')
