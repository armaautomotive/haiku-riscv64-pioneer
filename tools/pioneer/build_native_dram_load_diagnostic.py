#!/usr/bin/env python3
"""Build ONLY exact LOAD-ONLY82 driver/client;PRIVATE bytes required;NEVER deploys."""
import argparse
import hashlib
import os
import pathlib
import re
import stat
import struct
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
pins = {'PioneerNativeBfpModelBandData.h': 'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
        'PioneerNativeBfpModelBandMetadata.h': 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
def private_unchanged():
    assert private.is_absolute() and not private.is_symlink() and stat.S_IMODE(private.stat().st_mode) == 0o700 and private.stat().st_uid == os.geteuid()
    for name, digest in pins.items():
        info = (private / name).lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
        assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
private_unchanged()
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
output.mkdir(mode=0o700)
subprocess.run([sys.executable, str(here / 'build_native_dram_load.py'), str(build), str(output / 'image')], check=True)
code = (output / 'image/text.bin').read_bytes();boot = (output / 'image/boot.bin').read_bytes()
assert hashlib.sha256(code).hexdigest() == '966507e23929a1d28f62f56de0ea3c3a6c9148d9ff8f59c84138f746eea779a0'
assert hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
words = struct.unpack('<' + 'I' * (len(code) // 4), code)
header = ['// Generated LOAD ONLY code,not source weights or device authority.', 'struct PioneerNativeDramLoadGeneratedImage {',
          'static constexpr unsigned Width=5120,Phases=0,LoadOnly=1,Boot=0x%08x;' % struct.unpack('<I', boot)[0],
          'static unsigned Word(unsigned i){static const unsigned words[]={']
for first in range(0, len(words), 8):
    header.append(','.join('0x%08x' % word for word in words[first:first + 8]) + ',')
header += ['};return i<sizeof(words)/4?words[i]:0;}', '};']
(output / 'PioneerNativeDramLoadGeneratedImage.h').write_text('\n'.join(header) + '\n')
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-');cxx = tool + 'g++'
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'), '-I' + str(sdk / 'headers'),
             '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here), '-I' + str(private)]
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
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010082ULL', '-DPIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC=1']
commands = []
def run(command):
    commands.append(command);subprocess.run(command, check=True, cwd=output)
driver = root / 'src/add-ons/kernel/drivers/misc'
source = driver / 'pioneer_native_dram_load_driver.cpp'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(source), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
cases = [('missing-role', ['-UPIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC']),
         ('wrong-role', ['-UPIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC', '-DPIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC=2']),
         ('missing-reload', ['-UTT_PIONEER_DRIVER_RELOAD']),
         ('math81-tag', ['-UPIONEER_DRIVER_BUILD_TAG', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010081ULL'])]
for macro in ('PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC', 'PIONEER_NATIVE_BFP_DIAGNOSTIC', 'PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC',
              'PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC', 'PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
              'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'):
    cases.append((macro, ['-D' + macro + '=1']))
cases.append(('fast-math', ['-ffast-math']))
for name, extra in cases:
    # Preprocessed PRIVATE bytes must never be saved/printed.
    result = subprocess.run(common + native + extra + includes + ['-E', str(source)], stdout=subprocess.DEVNULL,
                            stderr=subprocess.PIPE, text=True, cwd=output)
    assert result.returncode and ('Build only the exact LOAD-ONLY82 reload role' in result.stderr
                                  or 'LOAD-ONLY82 cannot alias math/old contexts,Work or CPU reference' in result.stderr)
(output / 'driver-role-refusals.txt').write_text('\n'.join('PASS ' + name for name, _ in cases) + '\n')
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
for obj, name in [('native.o', 'tenstorrent_pci_probe-native-dram-load82'), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
binary = output / 'tenstorrent_pci_probe-native-dram-load82'
exports = subprocess.check_output([tool + 'nm', '-D', str(binary)], text=True)
(output / 'native-exports.txt').write_text(exports)
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= {r.split()[-1] for r in exports.splitlines() if r.split()}
legacy = 'PioneerNativeDramLoadLegacyFindDevice'
disassembly = subprocess.check_output([tool + 'objdump', '-d', '--disassemble=_Z' + str(len(legacy)) + legacy + 'PKc', str(binary)], text=True)
(output / 'native-legacy-entry.disassembly').write_text(disassembly)
instructions = []
for row in disassembly.splitlines():
    match = re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$', row)
    if match:
        instructions.append((match[1], match[2].strip()))
assert instructions == [('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'), ('ld', 's0,8(sp)'), ('li', 'a0,0'), ('add', 'sp,sp,16'), ('ret', '')]
assert hashlib.sha256((output / 'tenstorrent_pci_probe-default43-control').read_bytes()).hexdigest() == '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
libs = ['-L' + str(path) for path in sorted((build / 'build_packages').glob('gcc_syslibs-*/lib'))]
run([cxx, '-std=c++17', '-O2', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror', *user_includes, '-I' + str(private),
     '-I' + str(root / 'headers/private/shared'), '-I' + str(root / 'headers/private/libroot'),
     '-B' + str(release / 'system/glue'), '-L' + str(release / 'system/libroot'), '-L' + str(release / 'kits'), *libs,
     str(here / 'pioneer_native_dram_load_diagnostic.cpp'), str(here / 'PioneerModelWeightCache.cpp'),
     str(release / 'kits/shared/libshared.a'), '-lbe', '-o', str(output / 'pioneer-native-dram-load-diagnostic')])
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
private_unchanged()
for name in ('tenstorrent_pci_probe-native-dram-load82', 'tenstorrent_pci_probe-default43-control', 'pioneer-native-dram-load-diagnostic'):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print('BUILD_ONLY full LOAD-ONLY82 native driver/client LINKED/15 forbidden roles/legacy NULL/default unchanged;PRIVATE weight-containing binaries NEVER publish;NO native endpoint/hardware/byte/math/model/speed proof')
