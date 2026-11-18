#!/usr/bin/env python3
"""Build the isolated Haiku projection comparison and its linked Tensix image."""
import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess

HERE = Path(__file__).resolve().parent
TAG = '2026111700010225'


def run(*command, env=None):
    subprocess.run(command, check=True, env=env)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('haiku_build', type=Path)
    parser.add_argument('new_output', type=Path)
    args = parser.parse_args()
    build, output = args.haiku_build.resolve(strict=True), args.new_output.absolute()
    output.mkdir(mode=0o700)
    image = output / 'image'
    env = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
    run('sh', HERE / 'build_qwen38_block_image.sh', build, image,
        env=dict(env, PIONEER_Q38_STREAM_IMAGE='16'))
    boot, text = (image / 'boot.bin').read_bytes(), (image / 'text.bin').read_bytes()
    if boot != struct.pack('<I', 0x0006c06f) or not 0 < len(text) <= 8192 or len(text) % 4:
        raise RuntimeError('projection image footprint changed')
    words = '\n'.join(f'\t0x{word:08x},' for word, in struct.iter_unpack('<I', text))
    (output / 'PioneerQwen35ProjectionImage.h').write_text(
        '#pragma once\n'
        'enum { TT_Q35_PROJECTION_BOOT=0x0006c06f };\n'
        'static inline uint32_t PioneerQ35ProjectionWord(unsigned word) {\n'
        ' static const uint32_t words[]={\n' + words + '\n };\n'
        ' return word<sizeof(words)/sizeof(*words)?words[word]:0;\n}\n',
        encoding='ascii')
    run('sh', HERE / 'build_qwen38_banks_driver.sh', build, output / 'driver',
        env=dict(env, PIONEER_Q38_PANEL_DRIVER='5', PIONEER_DRIVER_RELOAD='1',
                 PIONEER_DRIVER_BUILD_TAG=TAG, PIONEER_Q35_PROJECTION_INCLUDE=str(output)))
    cxx = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
    objects = build / 'objects/haiku/riscv64'
    sdk = objects / 'packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release = objects / 'release'
    includes = ['-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'),
                '-I' + str(HERE.parents[1] / 'headers/private/libroot')]
    os_headers = sdk / 'headers/os'
    for include in (os_headers, *sorted(os_headers.iterdir())):
        if include.is_dir():
            includes += ['-isystem', str(include)]
    libraries = ['-L' + str(path) for path in sorted(build.glob('build_packages/gcc_syslibs-*/lib'))]
    client = output / 'pioneer-q35-projection-validate'
    run(cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
        '-DTT_PIONEER_EXPERIMENTAL_Q38_ROWS', '-DTT_PIONEER_Q35_PROJECTION_PROBE',
        '-DPIONEER_Q38_STREAM_DIAGNOSTICS', '-DPIONEER_Q38_PRIMITIVE_TIMING',
        *includes, '-B' + str(release / 'system/glue') + '/', '-L' + str(release / 'system/libroot'),
        *libraries, HERE / 'pioneer_q38_stream_probe.cpp', HERE / 'PioneerModelWeightCache.cpp',
        '-o', client, env=env)
    for path in (output / 'driver/tenstorrent_pci_probe-driver50', client, image / 'text.bin'):
        print(hashlib.sha256(path.read_bytes()).hexdigest(), path)


if __name__ == '__main__':
    main()
