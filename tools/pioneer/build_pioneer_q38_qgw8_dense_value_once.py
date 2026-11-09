#!/usr/bin/env python3
"""Derive the pinned tag134 one-shot parent; never run or install it."""
import hashlib
import os
from pathlib import Path
import sys
if len(sys.argv)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('absolute pinned tag133 parent and new absolute output required')
parent,output=map(Path,sys.argv[1:]);root=Path(__file__).resolve().parent
if not parent.is_absolute() or parent.is_symlink() or not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('invalid paths')
raw=parent.read_bytes();gate=root/'pioneer_q38_qgw8_dense_value_output.awk'
if hashlib.sha256(raw).hexdigest()!='0eb7e4e8d39f49fc023d8665751d7a82ce2470c79e4ad25f70d0c82b3d35d7fa':
    raise SystemExit('source parent identity refused')
if hashlib.sha256(gate.read_bytes()).hexdigest()!='f4747b8767b9ec6a0bbf7bb6b114f4b73d1b4d6f028040647f487af7a359128f':
    raise SystemExit('value gate identity refused')
text=raw.decode('utf-8')
changes=(
 ('pioneer-qgw8-dense-input-trial-20261109-a','pioneer-qgw8-dense-value-trial-20261109-a',1),
 ('tenstorrent_pci_probe-qgw8-dense-input-133','tenstorrent_pci_probe-qgw8-dense-value-134',1),
 ('pioneer-q38-qgw8-dense-input-candidate133','pioneer-q38-qgw8-dense-value-candidate134',1),
 ('pioneer_q38_qgw8_dense_input_output.awk','pioneer_q38_qgw8_dense_value_output.awk',1),
 ('02208326fff242a9300dd8cc33cdf84ca29282d7ab13d572b4144e58b7eb75e5','22c950146337def4ae733763cf50a2e8411ea3ba6313bfd090913fda3cb3017f',1),
 ('98773555089f139f21c398bc379518aca7c1bf82d93bcf3753dcf1ab00f99e29','eaa35bfbeed892e30128523b195e8ec29a9a1a0f6db724cbafb83f5e28117a4a',1),
 ('31525bf1ba1712d44eff08b341ce30743350ff8db4606486b5ea0519153f45d4','f4747b8767b9ec6a0bbf7bb6b114f4b73d1b4d6f028040647f487af7a359128f',1),
 ('metadata 2026102100010012 10','metadata 2026102100010012 12',2),
 ('2026102100010012 10 2026110700010133 11','2026102100010012 12 2026110700010134 13',1),
 ('metadata 2026110700010133 11','metadata 2026110700010134 13',2),
 ('2026110700010133 11 2026102100010012 12','2026110700010134 13 2026102100010012 14',1),
 ('metadata 2026102100010012 12 && health || exit 73','metadata 2026102100010012 14 && health || exit 73',1),
 ('QGW8_DENSE_INPUT_CHECK_ONLY_PASSED','QGW8_DENSE_VALUE_CHECK_ONLY_PASSED',1),
 ('QGW8_DENSE_INPUT_ONE_SHOT_CLOSED_STABLE_RESTORED','QGW8_DENSE_VALUE_ONE_SHOT_CLOSED_STABLE_RESTORED',1),
)
for old,new,count in changes:
    if text.count(old)!=count:raise SystemExit('unexpected parent shape: '+old)
    text=text.replace(old,new)
data=text.encode('utf-8')
with output.open('xb') as target:
    if target.write(data)!=len(data):raise OSError('partial parent file is untrusted')
print('DENSE_VALUE_PARENT_SHA256',hashlib.sha256(data).hexdigest(),'BUILD ONLY; NOT RUN')
