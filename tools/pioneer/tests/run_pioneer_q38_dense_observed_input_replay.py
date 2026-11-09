#!/usr/bin/env python3
"""Replay hash-checked captured card input through CPU projection, offline."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile

here=Path(__file__).resolve().parent
workspace=here.parents[2]
prior=workspace/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/trace.bin'
model=workspace/'generated.pioneer/models/Qwen3.8-27B-Q4_K_M.gguf'
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('candidate',type=Path);p.add_argument('--candidate-sha256',required=True)
p.add_argument('--snapshot-log',type=Path,required=True)
args=p.parse_args()
if any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):raise SystemExit('role override refused')
if not re.fullmatch(r'[0-9a-f]{64}',args.candidate_sha256):raise SystemExit('candidate hash refused')
log=args.snapshot_log
if not log.is_absolute() or log.is_symlink() or not log.is_file() or log.stat().st_size>1048576:
    raise SystemExit('snapshot log refused')
lines=log.read_text().splitlines()
completion='HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated'
snapshot='QGW8_DENSE_INPUT_SNAPSHOT sha256='+args.candidate_sha256+' bytes=1310720 tag=0x2026110700010133 generation=11; DATA_ONLY_AFTER_CLOSE'
if lines.count(completion)!=1 or lines.count(snapshot)!=1 or lines.index(snapshot)!=lines.index(completion)+1:
    raise SystemExit('closed tag133 snapshot record refused')
for path,digest in ((prior,'cf9448bb92349b5e38409ee02e6e80abc9824f2cf2454f18d8633fd5ff1f9ae1'),
                    (args.candidate,args.candidate_sha256)):
    if not path.is_absolute() or path.is_symlink() or path.stat().st_size!=64*5120*4 or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise SystemExit('trace identity refused: '+str(path))
if prior.read_bytes()[2*5120*4:]!=args.candidate.read_bytes()[2*5120*4:]:
    raise SystemExit('cross-run layers 2-63 differ; no cross-run projection comparison')
source=(here/'pioneer_q38_cpu_model_reference.cpp').read_text()
start=source.index('struct Q38CpuModelReference::Impl {')
end=source.index('\nQ38CpuModelReference::Q38CpuModelReference()',start)
implementation=source[start:end].replace('struct Q38CpuModelReference::Impl {','struct Q38LayerReplay {',1)
with tempfile.TemporaryDirectory(prefix='qgw8-observed-input-replay-',dir='/private/tmp') as tmp:
    root=Path(tmp);(root/'actual_cpu_replay_impl.inc').write_text(implementation);binary=root/'replay'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
                    '-I'+str(root),str(here/'pioneer_q38_dense_observed_input_replay.cpp'),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),str(here.parent/'PioneerModelWeightCache.cpp'),
                    '-o',str(binary)],check=True)
    subprocess.run([str(binary),str(model),str(args.candidate),str(prior)],check=True)
