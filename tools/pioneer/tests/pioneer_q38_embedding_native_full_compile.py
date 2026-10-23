#!/usr/bin/env python3
"""Old BYTE controls + ACTUAL new native source/zero/embedding,OBJECT ONLY."""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys

here = pathlib.Path(__file__).resolve().parent
directory = here.parent
if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] not in ('--rms', '--projection')):
    raise SystemExit('expected read-only SDK and NEW ABSOLUTE output [--rms|--projection]')
projection = sys.argv[-1] == '--projection'
rms = len(sys.argv) == 4
sdk, output = map(pathlib.Path, sys.argv[1:3])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
output.mkdir(mode=0o700)
role = dict(re.findall(r'\b(PIONEER_[A-Z0-9_]+)=([0-9]+)', (directory / 'build_qwen38_ffn_native_graph.sh').read_text()))
role.update(PIONEER_DRIVER_BUILD_TAG='2026102200010038', PIONEER_Q38_FFN_HOST_PROFILE='1', PIONEER_Q38_FFN_SEALED_PROOF='1')
clean = {key: value for key, value in os.environ.items() if not key.startswith('PIONEER_')}
trace = output / 'old-tag38.trace'
with trace.open('w') as log:
    subprocess.run(['sh', '-x', str(directory / 'build_qwen38_banks_driver.sh'), str(sdk), str(output / 'old-tag38')],
                   env={**clean, **role}, stdout=log, stderr=subprocess.STDOUT, check=True)
for name, sha in [('tenstorrent_pci_probe-driver55', '23e2e68a33c16f990552bb9bf146252a0ce9d23a4a3c3ec9f7f751c8dba77483'),
                  ('pioneer_p100a_reload_guard', 'd1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229'),
                  ('tenstorrent_pci_probe-default43-control', '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc')]:
    assert hashlib.sha256((output / 'old-tag38' / name).read_bytes()).hexdigest() == sha, name
driver = here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
commands = []
for line in trace.read_text().splitlines():
    if line.startswith('+ '):
        args = shlex.split(line[2:])
        if '-c' in args and args[args.index('-c') + 1] == str(driver) and '-DTT_PIONEER_DRIVER_RELOAD' in args:
            commands.append(args)
assert len(commands) == 1
args = commands[0]
prefix = 'projection-execution' if projection else 'learned-rms' if rms else 'embedding'
args[args.index('-c') + 1] = str(here / ('pioneer_q38_' + prefix.replace('-', '_') + '_native_full_compile.cpp'))
args[args.index('-o') + 1] = str(output / (prefix + '-native-COMPILE-ONLY.o'))
subprocess.run(args, check=True, env=clean)
frames = []
controller_frames = []
for line in (output / (prefix + '-native-COMPILE-ONLY.su')).read_text().splitlines():
    if len(line.split('\t')) != 3:
        continue
    frame = int(line.split('\t')[1])
    if ('PioneerQ38ProjectionGraphOwner.h:' if projection else 'PioneerQ38LearnedRmsController.h:' if rms else 'PioneerQ38EmbeddingController.h:') in line:
        controller_frames.append(frame)
    if any(name in line for name in ('PioneerQ38EmbeddingController.h:', 'PioneerQ38ProjectionZeroController.h:',
                                    'pioneer_embedding_native_kernel_', 'BindQ38EmbeddingModel', 'InitializeQ38EmbeddingModel',
                                    'RunQ38EmbeddingModel', 'PioneerSessionIO::Embedding', 'PioneerSessionIO::ProjectionZero',
                                    'PioneerQ38LearnedRmsController.h:', 'pioneer_rms_native_kernel_',
                                    'RunQ38LearnedRmsModel', 'PioneerSessionIO::LearnedRms',
                                    'PioneerQ38ProjectionGraphOwner.h:', 'pioneer_projection_native_kernel_',
                                    'Q38ProjectionExecution', 'PioneerSessionIO::ProjectionExecution', 'PioneerSessionIO::ProjectionGraph')):
        frames.append(frame)
assert frames and max(frames) <= 2048
assert controller_frames
print('PASS ACTUAL full driver new canonical binding/zero2MiB/root memory/QPE1' + ('/QRN1' if rms else '') + ('/QPR1+ALL119cohort' if projection else '') + ' execution/retention/END compile;maxNewKernelFrame='
      + str(max(frames)) + '/controller=' + str(max(controller_frames))
      + ';old driver/guard/default43 BYTE-IDENTICAL;NO_NEW_LINKED_DRIVER_NATIVE_SELECTOR_OR_INSTALL')
