#!/usr/bin/env python3
"""Cross-build the isolated 4B Work driver, ROOT image and Haiku client."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TAG = '2026112000010098'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--transfer-diagnostic', action='store_true',
                        help='capture the pending fixed NoC transfer on ROOT fault')
    parser.add_argument('--device-opt-level', choices=('s', '1', '2', '3'), default='s',
                        help='Tensix ROOT C++ optimization level (default: s)')
    parser.add_argument('--peer-opt-level', choices=('s', '1', '2', '3'), default='s',
                        help='Tensix peer C++ optimization level (default: s)')
    parser.add_argument('--batch-fetch', action='store_true',
                        help='batch complete original Qwen3.5 weight tiles in an isolated Work candidate')
    parser.add_argument('--batch-prefetch', action='store_true',
                        help='overlap double-buffered batches of original peer weight tiles with Tensix compute')
    parser.add_argument('--peer-tile-prefetch', action='store_true',
                        help='overlap checked peer weight-tile reads with Tensix matrix work')
    parser.add_argument('--peer-exec-profile', action='store_true',
                        help='record advisory peer execution-cycle telemetry for Work diagnostics')
    parser.add_argument('haiku_build', type=Path)
    parser.add_argument('new_output', type=Path)
    args = parser.parse_args()
    build = args.haiku_build.resolve(strict=True)
    output = args.new_output.absolute()
    output.mkdir(mode=0o700)
    env = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
    image = output / 'image'
    image_env = dict(env, PIONEER_Q35_WORK_OPT_LEVEL=args.device_opt_level,
                     PIONEER_Q35_WORK_PEER_OPT_LEVEL=args.peer_opt_level)
    if args.batch_fetch:
        image_env['PIONEER_Q35_WORK_BATCH_FETCH'] = '1'
    if args.batch_prefetch:
        image_env['PIONEER_Q35_WORK_BATCH_FETCH'] = '1'
        image_env['PIONEER_Q35_WORK_BATCH_PREFETCH'] = '1'
    if args.peer_tile_prefetch:
        image_env['PIONEER_Q35_WORK_TILE_PREFETCH'] = '1'
    if args.peer_exec_profile:
        image_env['PIONEER_Q35_WORK_PEER_EXEC_PROFILE'] = '1'
    if args.transfer_diagnostic:
        image_env['PIONEER_Q35_TRANSFER_DIAGNOSTIC'] = '1'
    subprocess.run(['sh', HERE / 'build_qwen35_work_image.sh', build, image], check=True, env=image_env)
    subprocess.run(['sh', HERE / 'build_qwen38_banks_driver.sh', build, output / 'driver'], check=True,
                   env=dict(env, PIONEER_Q38_PANEL_DRIVER='10', PIONEER_DRIVER_RELOAD='1',
                            PIONEER_DRIVER_BUILD_TAG=TAG, PIONEER_Q38_SPAN_PRELOAD='1',
                            PIONEER_Q38_SPAN_FAST='1', PIONEER_Q35_WORK_INCLUDE=str(image)))
    cxx = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
    objects = build / 'objects/haiku/riscv64'
    sdk = objects / 'packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release = objects / 'release'
    llama = ROOT / 'src/libs/llama.cpp'
    includes = ['-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'),
                '-I' + str(ROOT / 'headers/private/libroot'),
                '-isystem', str(llama / 'include'), '-isystem', str(llama / 'ggml/include')]
    os_headers = sdk / 'headers/os'
    for include in (os_headers, *sorted(os_headers.iterdir())):
        if include.is_dir():
            includes += ['-isystem', str(include)]
    libraries = ['-L' + str(path) for path in sorted(build.glob('build_packages/gcc_syslibs-*/lib'))]
    client = output / 'pioneer-q35-work'
    subprocess.run([cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-ffp-contract=off',
                    *includes, '-B' + str(release / 'system/glue') + '/',
                    '-L' + str(release / 'system/libroot'), *libraries,
                    HERE / 'pioneer_qwen35_work.cpp', HERE / 'PioneerModelWeightCache.cpp',
                    '-o', client], check=True, env=env)
    print(hashlib.sha256(client.read_bytes()).hexdigest(), client)


if __name__ == '__main__':
    main()
