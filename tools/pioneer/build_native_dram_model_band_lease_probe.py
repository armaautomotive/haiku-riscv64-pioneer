#!/usr/bin/env python3
"""COMPILE ONLY new lease in actual kernel SDK. NO native binding/device proof."""
import argparse
import hashlib
import os
import pathlib
import stat
import struct
import subprocess
if not __debug__:
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('private_export', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
build, private, output = args.haiku_build, args.private_export, args.output
assert build.is_absolute() and build.is_dir() and private.is_absolute() and not private.is_symlink()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
identity = private.stat()
assert stat.S_ISDIR(identity.st_mode) and identity.st_uid == os.geteuid() and stat.S_IMODE(identity.st_mode) == 0o700
pins = {'PioneerNativeBfpModelBandData.h': 'cf8e2ca4d8489b1e2c9dbf3d23486a5b2b53a678f042629cbb4385e68daaa4f5',
        'PioneerNativeBfpModelBandMetadata.h': 'a2c977bc4b7d517b5bc7f0ed188ed92eff16b3bc27dd0a59c5899e235c3d439c'}
for name, pin in pins.items():
    info = (private / name).lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid() and stat.S_IMODE(info.st_mode) == 0o600
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == pin
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
output.mkdir(mode=0o700)
subprocess.run(['python3', str(here / 'build_native_dram_band.py'), str(build), str(output / 'image'), '5120'], check=True)
boot = (output / 'image/boot.bin').read_bytes()
text = (output / 'image/text.bin').read_bytes()
assert hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
assert hashlib.sha256(text).hexdigest() == 'cd924896986dabcead8493c9dedca1d7b25fde323fbd972caf67ad93139a9309'
words = struct.unpack('<' + 'I' * (len(text) // 4), text)
header = ['// Generated code ONLY,not learned weights or owner authority.', 'struct PioneerNativeDramBandLeaseProbeImage {',
          'static constexpr unsigned Width=5120,Phases=4,Boot=0x%08x;' % struct.unpack('<I', boot)[0],
          'static unsigned Word(unsigned i){static const unsigned words[]={']
for first in range(0, len(words), 8):
    header.append(','.join('0x%08x' % word for word in words[first:first + 8]) + ',')
header += ['};return i<sizeof(words)/4?words[i]:0;}', '};']
(output / 'PioneerNativeDramBandLeaseProbeImage.h').write_text('\n'.join(header) + '\n')
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here), '-I' + str(private)]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
command = [tool + 'g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti',
           '-fstack-usage', '-ffreestanding', '-fno-strict-aliasing', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE',
           '-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010081ULL',
           '-DPIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC=1', *includes, '-c',
           str(here / 'tests/pioneer_native_dram_model_band_kernel_probe.cpp'), '-o', str(output / 'lease.o')]
subprocess.run(command, check=True, cwd=output)
(output / 'build-argv.txt').write_text('\n'.join(command) + '\n')
(output / 'unresolved.txt').write_bytes(subprocess.check_output([tool + 'nm', '-u', str(output / 'lease.o')]))
for name, pin in pins.items():
    assert hashlib.sha256((private / name).read_bytes()).hexdigest() == pin
print('PASS actual kernel SDK full Run/End template instantiation/Werror;NO native IO definitions/link/device entry or PCI/VM/FD/clock/driver/hardware authority')
print('COMPILE_ONLY object SHA', hashlib.sha256((output / 'lease.o').read_bytes()).hexdigest())
print('PRIVATE_HEADERS_UNCHANGED;object/generated PRIVATE build files MUST NOT be published:', output)
