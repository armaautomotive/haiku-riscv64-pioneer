#!/usr/bin/env python3
"""Generate a pinned one-shot tag132 shell parent, never run or install it."""
import hashlib
import os
from pathlib import Path
import sys

if len(sys.argv)!=2 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('one new absolute output path required')
output=Path(sys.argv[1]);root=Path(__file__).resolve().parent
if not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('new absolute output required')
parent=root/'pioneer_q38_qgw8_trace_physical_once.sh'
gate=root/'pioneer_q38_qgw8_dense_stage_output.awk'
raw=parent.read_bytes()
if hashlib.sha256(raw).hexdigest()!='4d127bdf20b02d0b5128bdbbe16b2ccc66bad736b3068fa5b4d94df1faace360':
    raise SystemExit('source parent identity refused')
if hashlib.sha256(gate.read_bytes()).hexdigest()!='00b042d648c82ae84838a8739f72e0657426813066cbd010393a79c8c493ce2d':
    raise SystemExit('stage gate identity refused')
text=raw.decode('utf-8')
changes=(
 ('pioneer-qgw8-trace-trial-20261109-a','pioneer-qgw8-dense-stage-trial-20261109-a',1),
 ('tenstorrent_pci_probe-qgw8-trace-131','tenstorrent_pci_probe-qgw8-dense-stage-132',1),
 ('pioneer-q38-qgw8-trace-candidate131','pioneer-q38-qgw8-dense-stage-candidate132',1),
 ('pioneer_q38_qgw8_trace_output.awk','pioneer_q38_qgw8_dense_stage_output.awk',1),
 ('e289d93fbecc569c249dacd1d72ed5d24eb73017ecfbc49c70c5f3e0a8a8e5e2','df4d8c24d3f3fd181e9433ae15af9141e7e73e3a78137e414ce1c704a3eacb53',1),
 ('34c6d9c21c3113d06979bc5e7feaffe6add4760a7b147d7fd6fd44f98f0207c3','5988d972f927990efd1bd4e89f3125b922fcf82bf216988c1499346d0fe1f15a',1),
 ('bd48c957eab697f3b1a8bba84bf746a04f67694480418245d90dc66ab23e5f9c','00b042d648c82ae84838a8739f72e0657426813066cbd010393a79c8c493ce2d',1),
 ('metadata 2026102100010012 6','metadata 2026102100010012 8',2),
 ('2026102100010012 6 2026110700010131 7','2026102100010012 8 2026110700010132 9',1),
 ('metadata 2026110700010131 7','metadata 2026110700010132 9',2),
 ('2026110700010131 7 2026102100010012 8','2026110700010132 9 2026102100010012 10',1),
 ('metadata 2026102100010012 8 && health || exit 73','metadata 2026102100010012 10 && health || exit 73',1),
 ('QGW8_TRACE_CHECK_ONLY_PASSED','QGW8_DENSE_STAGE_CHECK_ONLY_PASSED',1),
 ('QGW8_TRACE_ONE_SHOT_CLOSED_STABLE_RESTORED','QGW8_DENSE_STAGE_ONE_SHOT_CLOSED_STABLE_RESTORED',1),
)
for old,new,count in changes:
    if text.count(old)!=count:raise SystemExit('unexpected parent shape: '+old)
    text=text.replace(old,new)
data=text.encode('utf-8')
with output.open('xb') as target:
    if target.write(data)!=len(data):raise OSError('partial parent file is untrusted')
print('DENSE_STAGE_PARENT_SHA256',hashlib.sha256(data).hexdigest(),'BUILD ONLY; NOT RUN')
