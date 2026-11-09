#!/usr/bin/env python3
"""Exercise replay gates with deliberately mislabeled data; not a card result."""
from pathlib import Path
import hashlib
import subprocess
import sys
import tempfile

here=Path(__file__).resolve().parent;root=here.parents[2]
trace=root/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/trace.bin'
oldlog=root/'generated.pioneer/qualification-inputs/qgw8-dense-stage-physical-20261109/trial.stdout'
sha='cf9448bb92349b5e38409ee02e6e80abc9824f2cf2454f18d8633fd5ff1f9ae1'
runner=here/'run_pioneer_q38_dense_observed_input_replay.py'
def run(log,digest=sha,candidate=trace):
    return subprocess.run([sys.executable,str(runner),str(candidate),'--candidate-sha256',digest,'--snapshot-log',str(log)],capture_output=True,text=True)
assert run(oldlog).returncode, 'tag132 transcript accepted as tag133'
with tempfile.TemporaryDirectory(prefix='qgw8-replay-fake-',dir='/private/tmp') as directory:
    log=Path(directory)/'fake.log'
    log.write_text('HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated\n'
                   'QGW8_DENSE_INPUT_SNAPSHOT sha256='+sha+' bytes=1310720 tag=0x2026110700010133 generation=11; DATA_ONLY_AFTER_CLOSE\n')
    assert run(log,'0'*64).returncode, 'wrong digest accepted'
    changed=bytearray(trace.read_bytes());changed[2*5120*4]^=1
    other=Path(directory)/'different-layer.bin';other.write_bytes(changed)
    other_sha=hashlib.sha256(changed).hexdigest()
    log.write_text('HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated\n'
                   'QGW8_DENSE_INPUT_SNAPSHOT sha256='+other_sha+' bytes=1310720 tag=0x2026110700010133 generation=11; DATA_ONLY_AFTER_CLOSE\n')
    refused=run(log,other_sha,other)
    assert refused.returncode and 'cross-run layers 2-63 differ' in refused.stderr
    log.write_text('HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated\n'
                   'QGW8_DENSE_INPUT_SNAPSHOT sha256='+sha+' bytes=1310720 tag=0x2026110700010133 generation=11; DATA_ONLY_AFTER_CLOSE\n')
    result=run(log)
    assert result.returncode==0 and 'CPU_ONLY' in result.stdout, result.stderr
print('PASS synthetic transcript/tag/hash refusals and CPU replay; input is deliberately NOT an attention capture')
