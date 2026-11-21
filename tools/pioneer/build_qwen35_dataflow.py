#!/usr/bin/env python3
"""Build the standalone Qwen35 dataflow driver and benchmark client."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TAG = '2026112100010115'


def verify_kernel_stack_budget(driver_directory):
    usage_file = driver_directory / 'tenstorrent_pci_probe.su'
    entries = []
    for line in usage_file.read_text().splitlines():
        fields = line.split('\t')
        if len(fields) < 3:
            continue
        try:
            frame = int(fields[-2])
        except ValueError:
            continue
        entries.append((fields[0], frame, fields[-1]))

    dataflow = [(name, frame, kind) for name, frame, kind in entries
                if 'Dataflow' in name or 'Qwen35Dataflow' in name
                or 'PioneerQ35BuildProjectionProgram' in name]
    if not dataflow:
        raise RuntimeError('driver stack report has no Qwen35 dataflow entries')
    too_large = [(name, frame) for name, frame, _ in dataflow if frame > 8192]
    if too_large:
        raise RuntimeError('Qwen35 frame exceeds 8192 bytes: ' + repr(too_large))

    required = {
        'control': 'status_t Qwen35DataflowControl(',
        'generation': 'DataflowGenerationSequence::Run(',
        'decoder': 'DataflowDecoderSequence::Run(',
        'dense': 'DataflowDenseLayerSequence::Run(',
        'recurrent': 'DataflowRecurrentLayerSequence::Run(',
        'ffn': 'DataflowFfnSequence::Run(',
        'authority_build': 'DataflowProjectionAuthority::BuildProjection(',
        'program_build': 'DataflowProgramBuilder::Build(',
        'schedule_build': 'PioneerQ35BuildProjectionProgram(',
        'service_projection': 'DataflowServiceOwner::RunProjection(',
        'owner_projection': 'DataflowProjectionOwner::RunProjection(',
        'audit_projection': 'PioneerSessionIO::AuditProjection(',
        'publish': 'PioneerSessionIO::PublishCommand(',
        'invocation': 'PioneerSessionIO::InvocationMatches(',
        'bind_core': 'DataflowProgramBuilder::BindCore(',
        'delta_build': 'DataflowDeltaProgramBuilder::Build(',
        'delta_bind': 'DataflowDeltaProgramBuilder::BindCore(',
        'delta_matches': 'DataflowProjectionAuthority::DeltaMatches(',
        'owner_delta': 'DataflowProjectionOwner::RunDelta(',
        'audit_delta': 'PioneerSessionIO::AuditDelta(',
        'delta_invocation': 'PioneerSessionIO::DeltaInvocationMatches(',
    }
    sizes = {}
    for key, marker in required.items():
        matches = [(frame, kind) for name, frame, kind in entries
                   if marker in name]
        if not matches and key == 'generation' and 'control' in sizes:
            # The compiler may fully inline this template into the ioctl
            # handler. Its stack usage is then included in the measured
            # Qwen35DataflowControl frame, and no separate .su record exists.
            sizes[key] = 0
            print('qwen35_generation_frame=inlined_into_control')
            continue
        if not matches or any(kind != 'static' for _, kind in matches):
            raise RuntimeError('missing or unbounded Qwen35 stack frame: ' + key)
        sizes[key] = max(frame for frame, _ in matches)

    # Deliberately sum the projection build and service execution branches,
    # even though they run one after the other. Leave 2.5 KiB for ioctl entry
    # and smaller helpers inside Haiku's 20 KiB kernel stack.
    request_stack = (sizes['control'] + sizes['generation'] + sizes['decoder']
                    + max(sizes['dense'], sizes['recurrent']) + sizes['ffn'])
    projection_branch = (sizes['authority_build'] + sizes['program_build']
                         + sizes['schedule_build'] + sizes['service_projection']
                         + sizes['owner_projection'] + sizes['audit_projection']
                         + sizes['publish'] + sizes['invocation']
                         + sizes['bind_core'])
    recurrent_request_stack = (sizes['control'] + sizes['generation']
                               + sizes['decoder'] + sizes['recurrent'])
    delta_service_branch = sizes['owner_delta'] + max(
        sizes['publish'], sizes['delta_invocation'],
        sizes['audit_delta'] + max(sizes['delta_matches'], sizes['delta_bind']))
    delta_build_branch = max(sizes['delta_build'], sizes['delta_matches'])
    delta_branch = recurrent_request_stack + max(
        delta_service_branch, delta_build_branch)
    bound = max(request_stack + projection_branch, delta_branch)
    if bound > 18432:
        raise RuntimeError(f'Qwen35 conservative kernel-stack bound is {bound} bytes')
    print(f'qwen35_kernel_stack_conservative_bytes={bound}/18432')


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
    verify_kernel_stack_budget(driver)

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
