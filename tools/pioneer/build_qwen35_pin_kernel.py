#!/usr/bin/env python3
"""Build an isolated Pioneer kernel with the experimental driver pin service.

This reuses the saved, verified Pioneer kernel build objects. It never changes
that build directory, installs a kernel, accesses the card, or boots anything.
"""
import argparse
import hashlib
import json
import os
import shlex
import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
BASE_SHA256 = '90d6a7f5ccb2a93f42bb0256b050dbd1363ed05bdb3169820676fa45b08e5de8'
DM = 'src/system/kernel/device_manager/device_manager.cpp'
MODULE = 'src/system/kernel/module.cpp'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checked_sources(mirror):
    dm = (REPO / DM).read_text()
    start = dm.index('#ifdef PIONEER_DEVICE_MANAGER_PIN_WITNESS\n')
    end = dm.index('#endif\n', start) + len('#endif\n')
    dm = dm[:start] + dm[end:]
    dm = dm.replace('// PIONEER_DM_INIT_LIFETIME_BEGIN\n', '').replace('// PIONEER_DM_INIT_LIFETIME_END\n', '')
    dm = dm.replace('static const char* sGenericContextPath;\n\n\n\n',
                    'static const char* sGenericContextPath;\n\n\n', 1)
    mod = (REPO / MODULE).read_text()
    mod = mod.replace('#ifdef PIONEER_DEVICE_MANAGER_PIN_WITNESS\nextern module_info gPioneerDeviceManagerPinModule;\n#endif\n', '')
    mod = mod.replace('#ifdef PIONEER_DEVICE_MANAGER_PIN_WITNESS\n\t&gPioneerDeviceManagerPinModule,\n#endif\n', '')
    if dm != (mirror / DM).read_text() or mod != (mirror / MODULE).read_text():
        raise RuntimeError('kernel source differs from the saved build beyond the pin-service changes')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path, help='saved generated.riscv64 directory')
    parser.add_argument('jam', type=Path, help='Haiku Jam executable')
    parser.add_argument('output', type=Path, help='new absolute output directory')
    args = parser.parse_args()
    build = args.build.resolve(strict=True)
    jam = args.jam.resolve(strict=True)
    if not args.output.is_absolute() or args.output.exists() or args.output.is_symlink():
        parser.error('output must be a new absolute path')
    output = args.output
    if output.resolve().is_relative_to(build):
        parser.error('output must be outside the saved build directory')
    mirror = (build / '../haiku').resolve(strict=True)
    checked_sources(mirror)
    kernel_dir = build / 'objects/haiku/riscv64/release/system/kernel'
    saved = kernel_dir / 'kernel_riscv64'
    if digest(saved) != BASE_SHA256:
        raise RuntimeError('saved Pioneer kernel hash does not match the verified baseline')
    output.mkdir(mode=0o700)
    actions = output / 'jam-actions.txt'
    env = dict(os.environ, HAIKU_REVISION='hrev99999')
    with (output / 'jam.log').open('w') as log:
        subprocess.run([str(jam), '-n', '-a', '-o', str(actions), '-q', 'kernel_riscv64'],
                       cwd=build, env=env, stdout=log, stderr=log, check=True)
    lines = actions.read_text().replace('\\\n', ' ').splitlines()

    def select(predicate):
        matches = [shlex.split(line) for line in lines if predicate(line)]
        if len(matches) != 1:
            raise RuntimeError('expected one matching saved build action, found ' + str(len(matches)))
        return matches[0]

    def run(command, label):
        with (output / (label + '.log')).open('w') as log:
            subprocess.run(command, cwd=build, stdout=log, stderr=log, check=True)

    def set_output(command, path):
        command[command.index('-o') + 1] = str(path)

    full_link = select(lambda x: ' --script=../haiku/src/system/ldscripts/riscv64/kernel.ld ' in x
                       and ' -shared ' in x)
    xres = build / 'objects/darwin/arm64/release/tools/xres'
    version = build / 'objects/darwin/arm64/release/tools/setversion'
    resource = build / 'objects/haiku/riscv64/common/system/kernel/kernel.rsrc'

    def finish(command, directory, label):
        directory.mkdir(mode=0o700)
        path = directory / 'kernel_riscv64'  # ELF version metadata uses this basename.
        command = list(command)
        set_output(command, path)
        run(command, label)
        run([str(xres), '-o', str(path), str(resource)], label + '-resources')
        run([str(version), str(path), '-system', '1', '0', '0', 'a', '1', '-short', 'Developer Build'],
            label + '-version')
        return path

    control = finish(full_link, output / 'control', 'control-link')
    if digest(control) != BASE_SHA256:
        raise RuntimeError('saved build objects did not reproduce the known kernel byte for byte')
    built = {}
    for name, rel in (('module', MODULE), ('device_manager', DM)):
        original = '../haiku/' + rel
        command = select(lambda x: ' -c "' + original + '" ' in x)
        command[command.index(original)] = str(REPO / rel)
        built[name] = output / (name + '.o')
        set_output(command, built[name])
        command.append('-DPIONEER_DEVICE_MANAGER_PIN_WITNESS')
        run(command, 'compile-' + name)
    merges = (
        ('core', ' -r objects/haiku/riscv64/release/system/kernel/boot_item.o ',
         'objects/haiku/riscv64/release/system/kernel/module.o', built['module']),
        ('device_manager', ' -r objects/haiku/riscv64/release/system/kernel/device_manager/AbstractModuleDevice.o ',
         'objects/haiku/riscv64/release/system/kernel/device_manager/device_manager.o', built['device_manager']),
    )
    merged = {}
    for name, needle, old, new in merges:
        command = select(lambda x: needle in x)
        command[command.index(old)] = str(new)
        merged[name] = output / (name + '-merged.o')
        set_output(command, merged[name])
        run(command, 'merge-' + name)
    candidate_link = list(full_link)
    for old, new in (
        ('objects/haiku/riscv64/release/system/kernel/kernel_core.o', merged['core']),
        ('objects/haiku/riscv64/release/system/kernel/device_manager/kernel_device_manager.o', merged['device_manager']),
    ):
        candidate_link[candidate_link.index(old)] = str(new)
    candidate = finish(candidate_link, output / 'candidate', 'candidate-link')
    nm = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    symbols = subprocess.check_output([str(nm), '-C', str(candidate)], text=True)
    if ' gPioneerDeviceManagerPinModule\n' not in symbols:
        raise RuntimeError('candidate lacks the experimental built-in pin service')
    table = [line.split() for line in symbols.splitlines() if line.endswith(' sBuiltInModules')]
    if len(table) != 1:
        raise RuntimeError('candidate built-in module table is missing or ambiguous')
    table_address = int(table[0][0], 16)
    readelf = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-readelf'
    relocations = subprocess.check_output([str(readelf), '-Wr', str(candidate)], text=True)
    def has_relocation(address, symbol):
        for line in relocations.splitlines():
            fields = line.split()
            if (len(fields) >= 5 and fields[0] == f'{address:016x}'
                    and fields[2] == 'R_RISCV_64' and fields[4].split('@')[0] == symbol):
                return True
        return False
    if (not has_relocation(table_address, 'gDeviceManagerModule')
            or not has_relocation(table_address + 8, 'gPioneerDeviceManagerPinModule')):
        raise RuntimeError('pin service is not registered in the built-in module table')
    control_symbols = subprocess.check_output([str(nm), str(control)], text=True)
    if 'gPioneerDeviceManagerPinModule' in control_symbols:
        raise RuntimeError('control unexpectedly contains the pin service')
    manifest = {
        'saved_kernel_sha256': digest(saved), 'reproduced_control_sha256': digest(control),
        'candidate_kernel_sha256': digest(candidate),
        'device_manager_source_sha256': digest(REPO / DM), 'module_source_sha256': digest(REPO / MODULE),
        'pin_header_sha256': digest(REPO / 'src/system/kernel/device_manager/PioneerDeviceManagerPinModule.h'),
        'jam_actions_sha256': digest(actions), 'installed_or_booted': False,
    }
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))
    print('EXPERIMENTAL KERNEL: not installed, not booted, no hardware qualification')


if __name__ == '__main__':
    main()
