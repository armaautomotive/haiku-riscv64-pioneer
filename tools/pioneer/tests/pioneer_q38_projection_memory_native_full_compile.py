#!/usr/bin/env python3
"""Old tag38 BYTE control + ACTUAL full-driver new memory compile ONLY.

Reuse the exact verified old compiler argv,not a different mock native IO.
No new linked driver,manifest/updater entry,upload/install or card access.
"""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
stashes = sys.argv[-1:] == ['--result-stashes']
if stashes:
    sys.argv.pop()
if len(sys.argv) != 3:
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output directory')
sdk, output = map(pathlib.Path, sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
native = (directory / 'build_qwen38_ffn_native_graph.sh').read_text()
role = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)', native))
role.update(PIONEER_DRIVER_BUILD_TAG='2026102200010038', PIONEER_Q38_FFN_HOST_PROFILE='1', PIONEER_Q38_FFN_SEALED_PROOF='1')
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
trace = output / 'old-tag38-build.trace'
with trace.open('w') as log:
    subprocess.run(['sh', '-x', str(directory / 'build_qwen38_banks_driver.sh'), str(sdk), str(output / 'old-tag38')],
                   env={**clean, **role}, stdout=log, stderr=subprocess.STDOUT, check=True)
for name, digest in [('tenstorrent_pci_probe-driver55', '23e2e68a33c16f990552bb9bf146252a0ce9d23a4a3c3ec9f7f751c8dba77483'),
                     ('pioneer_p100a_reload_guard', 'd1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229'),
                     ('tenstorrent_pci_probe-default43-control', '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc')]:
    assert hashlib.sha256((output / 'old-tag38' / name).read_bytes()).hexdigest() == digest, name
commands = []
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
for line in trace.read_text().splitlines():
    if not line.startswith('+ '):
        continue
    args = shlex.split(line[2:])
    if '-c' in args and args[args.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in args:
        commands.append(args)
assert len(commands) == 1
args = commands[0]
args[args.index('-c') + 1] = str(here / 'pioneer_q38_projection_memory_native_full_compile.cpp')
args[args.index('-o') + 1] = str(output / 'new-memory-COMPILE-ONLY.o')
subprocess.run(args, check=True, env=clean)
usage = (output / 'new-memory-COMPILE-ONLY.su').read_text().splitlines()
frames = [int(line.split('\t')[1]) for line in usage if ('PioneerQ38ProjectionGraphMemory.h:' in line or 'pioneer_projection_memory_kernel_' in line) and len(line.split('\t')) == 3]
assert frames and max(frames) <= 2048
print('PASS ACTUAL full driver/context/scope/retention/END/new memory Prepare+Close RISC-V kernel compile;maxNewFrame=' + str(max(frames))
      + ';old tag38/guard/default43 BYTE-IDENTICAL;NO_NEW_LINKED_DRIVER_NATIVE_SELECTOR_INSTALL_OR_EXECUTION')
if stashes:
    args[args.index('-c') + 1] = str(here / 'pioneer_q38_projection_execution_native_full_compile.cpp')
    args[args.index('-o') + 1] = str(output / 'old-execution-CONTROL.o')
    subprocess.run(args, check=True, env=clean)
    assert hashlib.sha256((output / 'old-execution-CONTROL.o').read_bytes()).hexdigest() == '2d127c1e3a36ad93a61aeff66b85d4d8ce9185c7ae2255da7f48b3d0e6024c35'
    args[args.index('-o') + 1] = str(output / 'new-stash-COMPILE-ONLY.o')
    subprocess.run([*args, '-DPIONEER_Q38_PROJECTION_RESULT_STASH=1'], check=True, env=clean)
    usage = (output / 'new-stash-COMPILE-ONLY.su').read_text().splitlines()
    def frame_map(lines):
        return {re.sub(r':\d+:\d+:', ':', line.split('\t')[0]): int(line.split('\t')[1])
                for line in lines if len(line.split('\t')) == 3}
    previous = frame_map((output / 'old-execution-CONTROL.su').read_text().splitlines())
    current = frame_map(usage)
    # Existing unrelated legacy ioctl/DRAM-probe frames are >2KiB in the
    # BYTE-IDENTICAL control. Do not hide growth: EVERY oversized frame
    # must already exist and not grow. ALL new projection frames <=2KiB.
    assert current
    for name, size in current.items():
        assert size <= 2048 or name in previous and size <= previous[name], (name, size, previous.get(name))
    frames = [size for name, size in current.items() if 'PioneerQ38Projection' in name or 'pioneer_projection_native_kernel_' in name]
    assert frames and max(frames) <= 2048
    print('PASS ACTUAL QPS1 native complete execution/RMS/source/restore/END compile maxFrame=' + str(max(frames))
          + ';ALL oversized legacy frames <=BYTE-IDENTICAL control;macro-OFF execution object BYTE-IDENTICAL2d127c1e;NO_NEW_LINKED_DRIVER_SELECTOR_INSTALL_OR_CARD')
