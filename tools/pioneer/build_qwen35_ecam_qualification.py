#!/usr/bin/env python3
"""Reproduce the recorded ECAM and build a fixed-probe candidate offline."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

REPO = Path(__file__).resolve().parents[2]
REL = Path('src/add-ons/kernel/busses/pci/ecam')
BASE = '23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d'
SOURCES = {
    'ECAMPCIController.cpp': 'c08cc6474977980b7da95b0ac24b3031529768b2316e06c3d50d9fa2c08337ba',
    'ECAMPCIController.h': '3e0d6376a938af2d7ae80e21be497b1cd788d08dc0f3ee3e606846c918a8f203',
    'ECAMPCIControllerACPI.cpp': 'c6db08da4a4df641e931fefe550b673a484a425ee2e5681d899dcd45ded59a8f',
    'ECAMPCIControllerFDT.cpp': '6a61955b718d262cf77397379bf24c33148a51411e94656931f5243464c83bcb',
    'kernel_interface.cpp': '88e85e1cc52d19b7a5bd0b27bc94cebec4d74fe6cfeae2bc4babc91c493135ec',
    'PioneerSg2042LeaseProbeModule.h': '813b365ff81292648d0d194d6e548c1e81b0eececc63e0e18b1116b7107ce9ac',
    'PioneerSg2042RecoveryModule.h': '39708d8c2cf420cb9afe027758ccfe2c223c58bd0bc780fe873db3dafcfb3c55',
    'Sg2042RecoveryLease.h': '88b3cd67577a20f19ace93b8c068b36934b169de3ef1e0b21ace65594727f8db',
    'Sg2042RootPolicy.h': 'c79178e8fd666b1ea23a139cd26fb9d0618942d45f092343a411ab93db938d4e',
}
UNITS = ('ECAMPCIController', 'ECAMPCIControllerACPI', 'ECAMPCIControllerFDT', 'kernel_interface')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('jam', type=Path)
    parser.add_argument('new_absolute_output', type=Path)
    args = parser.parse_args()
    build = args.build.resolve(strict=True)
    jam = args.jam.resolve(strict=True)
    out = args.new_absolute_output
    if not out.is_absolute() or out.exists() or out.is_symlink() or out.resolve().is_relative_to(build):
        parser.error('output must be new, absolute, and outside the saved build')
    for name, expected in SOURCES.items():
        if digest(REPO / REL / name) != expected:
            raise RuntimeError('unverified qualification source: ' + name)
    saved = build / 'objects/haiku/riscv64/release/add-ons/kernel/busses/pci/ecam/ecam'
    if digest(saved) != BASE:
        raise RuntimeError('saved ECAM does not match the recorded recovery controller')
    out.mkdir(mode=0o700)
    actions = out / 'jam-actions.txt'
    with (out / 'jam.log').open('w') as log:
        subprocess.run([str(jam), '-n', '-a', '-o', str(actions), '-q', '<pci>ecam'],
                       cwd=build, env=dict(os.environ, HAIKU_REVISION='hrev99999'),
                       stdout=log, stderr=log, check=True)
    lines = actions.read_text().replace('\\\n', ' ').splitlines()

    def select(predicate):
        matches = [shlex.split(line) for line in lines if predicate(line)]
        if len(matches) != 1:
            raise RuntimeError('expected exactly one Jam action; got ' + str(len(matches)))
        return matches[0]

    def run(command, name):
        with (out / (name + '.log')).open('w') as log:
            subprocess.run(command, cwd=build, stdout=log, stderr=log, check=True)

    original = select(lambda line: ' -soname="ecam" ' in line and ' -shared ' in line)
    tool = build / 'objects/darwin/arm64/release/tools/setversion'
    version_env = dict(os.environ, DYLD_LIBRARY_PATH=str(build / 'objects/darwin/lib'))

    def finish(command, directory, label):
        directory.mkdir(mode=0o700)
        target = directory / 'ecam'
        command = list(command)
        command[command.index('-o') + 1] = str(target)
        run(command, label + '-link')
        with (out / (label + '-version.log')).open('w') as log:
            subprocess.run([str(tool), str(target), '-system', '1', '0', '0', 'a', '1',
                            '-short', 'Developer Build'], cwd=build, env=version_env,
                           stdout=log, stderr=log, check=True)
        return target

    baseline = finish(original, out / 'baseline', 'baseline')
    if digest(baseline) != BASE or baseline.read_bytes() != saved.read_bytes():
        raise RuntimeError('saved build objects did not reproduce ECAM byte for byte')
    candidate_link = list(original)
    for name in UNITS:
        original_source = '../haiku/' + str(REL / (name + '.cpp'))
        command = select(lambda line: ' -c "' + original_source + '" ' in line)
        command[command.index(original_source)] = str(REPO / REL / (name + '.cpp'))
        obj = out / (name + '.o')
        command[command.index('-o') + 1] = str(obj)
        command.extend(['-DPIONEER_SG2042_RECOVERY_WITNESS', '-DPIONEER_SG2042_RECOVERY_QUALIFICATION'])
        run(command, 'compile-' + name)
        old_obj = 'objects/haiku/riscv64/release/add-ons/kernel/busses/pci/ecam/' + name + '.o'
        candidate_link[candidate_link.index(old_obj)] = str(obj)
    candidate = finish(candidate_link, out / 'qualification', 'qualification')
    nm = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    objdump = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump'
    symbols = subprocess.check_output([str(nm), '-C', str(candidate)], text=True)
    if 'gPioneerLeaseProbe' not in symbols or 'gPioneerRecovery' not in symbols:
        raise RuntimeError('qualification probe or fail-closed recovery module missing')
    disassembly = subprocess.check_output([str(objdump), '-Cd', str(out / 'kernel_interface.o')], text=True)
    refusal = re.search(r'<gPioneerRecovery.*?_FUN.*?>:(.*?)(?:\n\n|\Z)', disassembly, re.S)
    if (not refusal or not re.search(r'\blui\s+a0,0x80000\b', refusal.group(1))
            or not re.search(r'\baddi?\s+a0,a0,15\b', refusal.group(1))
            or not re.search(r'\bret\b', refusal.group(1))
            or re.search(r'\b(?:call|jal|jalr|j|jr|b(?:eq|ne|lt|ge|ltu|geu|eqz|nez))\b', refusal.group(1))):
        raise RuntimeError('general recovery callback is not the refusing witness')
    manifest = {'recorded_ecam_sha256': BASE, 'reproduced_ecam_sha256': digest(baseline),
                'candidate_ecam_sha256': digest(candidate), 'source_sha256': SOURCES,
                'jam_actions_sha256': digest(actions), 'installed_or_booted': False}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))
    print('PASS recorded ECAM reproduced; fixed-probe candidate built, not installed or booted')


if __name__ == '__main__':
    main()
