#!/usr/bin/env python3
"""Build the standalone Qwen35 dataflow driver and benchmark client."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TAG = '2026112100010106'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('haiku_build', type=Path)
    parser.add_argument('new_output', type=Path)
    args = parser.parse_args()

    build = args.haiku_build.resolve(strict=True)
    output = args.new_output.absolute()
    output.mkdir(mode=0o700)
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PIONEER_')}

    driver = output / 'driver'
    driver_env = dict(env, PIONEER_Q38_PANEL_DRIVER='15',
                      PIONEER_DRIVER_RELOAD='1',
                      PIONEER_DRIVER_BUILD_TAG=TAG,
                      PIONEER_Q38_SPAN_PRELOAD='1',
                      PIONEER_Q38_SPAN_FAST='1')
    subprocess.run(['sh', HERE / 'build_qwen38_banks_driver.sh', build, driver],
                   check=True, env=driver_env)

    cxx = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
    objects = build / 'objects/haiku/riscv64'
    sdk = objects / 'packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release = objects / 'release'
    llama = ROOT / 'src/libs/llama.cpp'
    includes = ['-I' + str(sdk / 'headers'), '-idirafter',
                str(sdk / 'headers/posix'),
                '-I' + str(ROOT / 'headers/private/libroot'),
                '-isystem', str(llama / 'include'),
                '-isystem', str(llama / 'ggml/include')]
    os_headers = sdk / 'headers/os'
    for include in (os_headers, *sorted(os_headers.iterdir())):
        if include.is_dir():
            includes += ['-isystem', str(include)]
    libraries = ['-L' + str(path)
                 for path in sorted(build.glob('build_packages/gcc_syslibs-*/lib'))]
    client = output / 'pioneer-q35-dataflow'
    subprocess.run([
        cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
        '-ffp-contract=off', *includes,
        '-B' + str(release / 'system/glue') + '/',
        '-L' + str(release / 'system/libroot'), *libraries,
        HERE / 'pioneer_qwen35_dataflow.cpp',
        HERE / 'PioneerModelWeightCache.cpp', '-o', client],
        check=True, env=env)

    driver_image = driver / 'tenstorrent_pci_probe-driver60'
    print('driver_sha256=' + hashlib.sha256(driver_image.read_bytes()).hexdigest())
    print('client_sha256=' + hashlib.sha256(client.read_bytes()).hexdigest())
    print('driver=' + str(driver_image))
    print('client=' + str(client))


if __name__ == '__main__':
    main()
