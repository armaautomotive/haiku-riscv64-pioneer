#!/usr/bin/env python3
"""Derive the pinned tag133 one-shot parent; never run or install it."""
import hashlib
import os
from pathlib import Path
import sys

if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute pinned tag132 parent and new absolute output required')
parent,output=map(Path,sys.argv[1:]);root=Path(__file__).resolve().parent
if not parent.is_absolute() or parent.is_symlink() or not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('invalid paths')
raw=parent.read_bytes();gate=root/'pioneer_q38_qgw8_dense_input_output.awk'
if hashlib.sha256(raw).hexdigest()!='3c653d6047e2bf82f477fa5b333e13b4380de67173a3dbf6fa78fc1c7bb2d9be':
    raise SystemExit('source parent identity refused')
if hashlib.sha256(gate.read_bytes()).hexdigest()!='31525bf1ba1712d44eff08b341ce30743350ff8db4606486b5ea0519153f45d4':
    raise SystemExit('input gate identity refused')
text=raw.decode('utf-8')
changes=(
 ('pioneer-qgw8-dense-stage-trial-20261109-a','pioneer-qgw8-dense-input-trial-20261109-a',1),
 ('tenstorrent_pci_probe-qgw8-dense-stage-132','tenstorrent_pci_probe-qgw8-dense-input-133',1),
 ('pioneer-q38-qgw8-dense-stage-candidate132','pioneer-q38-qgw8-dense-input-candidate133',1),
 ('pioneer_q38_qgw8_dense_stage_output.awk','pioneer_q38_qgw8_dense_input_output.awk',1),
 ('df4d8c24d3f3fd181e9433ae15af9141e7e73e3a78137e414ce1c704a3eacb53','02208326fff242a9300dd8cc33cdf84ca29282d7ab13d572b4144e58b7eb75e5',1),
 ('5988d972f927990efd1bd4e89f3125b922fcf82bf216988c1499346d0fe1f15a','98773555089f139f21c398bc379518aca7c1bf82d93bcf3753dcf1ab00f99e29',1),
 ('00b042d648c82ae84838a8739f72e0657426813066cbd010393a79c8c493ce2d','31525bf1ba1712d44eff08b341ce30743350ff8db4606486b5ea0519153f45d4',1),
 ('metadata 2026102100010012 8','metadata 2026102100010012 10',2),
 ('2026102100010012 8 2026110700010132 9','2026102100010012 10 2026110700010133 11',1),
 ('metadata 2026110700010132 9','metadata 2026110700010133 11',2),
 ('2026110700010132 9 2026102100010012 10','2026110700010133 11 2026102100010012 12',1),
 ('metadata 2026102100010012 10 && health || exit 73','metadata 2026102100010012 12 && health || exit 73',1),
 ('QGW8_DENSE_STAGE_CHECK_ONLY_PASSED','QGW8_DENSE_INPUT_CHECK_ONLY_PASSED',1),
 ('QGW8_DENSE_STAGE_ONE_SHOT_CLOSED_STABLE_RESTORED','QGW8_DENSE_INPUT_ONE_SHOT_CLOSED_STABLE_RESTORED',1),
)
for old,new,count in changes:
    if text.count(old)!=count:raise SystemExit('unexpected parent shape: '+old)
    text=text.replace(old,new)
data=text.encode('utf-8')
with output.open('xb') as target:
    if target.write(data)!=len(data):raise OSError('partial parent file is untrusted')
print('DENSE_INPUT_PARENT_SHA256',hashlib.sha256(data).hexdigest(),'BUILD ONLY; NOT RUN')
