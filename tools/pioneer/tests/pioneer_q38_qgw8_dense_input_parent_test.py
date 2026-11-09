#!/usr/bin/env python3
"""Run the existing one-shot fault suite against the actual tag133 parent."""
from pathlib import Path
import subprocess
import sys
import tempfile

here=Path(__file__).resolve().parent
original=here/'pioneer_q38_qgw8_trace_parent_test.py'
with tempfile.TemporaryDirectory(prefix='qgw8-input-parent-wrapper-',dir='/private/tmp') as directory:
    old=Path(directory)/'stage.sh';parent=Path(directory)/'input.sh'
    subprocess.run([sys.executable,str(here.parent/'build_pioneer_q38_qgw8_dense_stage_once.py'),str(old)],check=True)
    subprocess.run([sys.executable,str(here.parent/'build_pioneer_q38_qgw8_dense_input_once.py'),str(old),str(parent)],check=True)
    subprocess.run(['sh','-n',str(parent)],check=True)
    text=original.read_text();source_line="source=(here.parent/'pioneer_q38_qgw8_trace_physical_once.sh').read_text()"
    assert text.count(source_line)==1
    text=text.replace(source_line,'source=Path('+repr(str(parent))+').read_text()')
    changes=(
     ('e289d93fbecc569c249dacd1d72ed5d24eb73017ecfbc49c70c5f3e0a8a8e5e2','02208326fff242a9300dd8cc33cdf84ca29282d7ab13d572b4144e58b7eb75e5'),
     ('34c6d9c21c3113d06979bc5e7feaffe6add4760a7b147d7fd6fd44f98f0207c3','98773555089f139f21c398bc379518aca7c1bf82d93bcf3753dcf1ab00f99e29'),
     ('bd48c957eab697f3b1a8bba84bf746a04f67694480418245d90dc66ab23e5f9c','31525bf1ba1712d44eff08b341ce30743350ff8db4606486b5ea0519153f45d4'),
     ('pioneer-qgw8-trace-trial-20261109-a','pioneer-qgw8-dense-input-trial-20261109-a'),
     ('tenstorrent_pci_probe-qgw8-trace-131','tenstorrent_pci_probe-qgw8-dense-input-133'),
     ('pioneer-q38-qgw8-trace-candidate131','pioneer-q38-qgw8-dense-input-candidate133'),
     ('pioneer_q38_qgw8_trace_output.awk','pioneer_q38_qgw8_dense_input_output.awk'),
     ('2026110700010131','2026110700010133'),
     ("adminstate.write_text('2026102100010012 6\\n')","adminstate.write_text('2026102100010012 10\\n')"),
     ('echo "$2 7"','echo "$2 11"'),('echo "$2 8"','echo "$2 12"'),
     ('QGW8_TRACE_SNAPSHOT','QGW8_DENSE_INPUT_SNAPSHOT'),('generation=7; DATA_ONLY_AFTER_CLOSE','generation=11; DATA_ONLY_AFTER_CLOSE'),
     ('QGW8_TRACE_CHECK_ONLY_PASSED','QGW8_DENSE_INPUT_CHECK_ONLY_PASSED'),
    )
    for old_text,new_text in changes:
        assert old_text in text,old_text
        text=text.replace(old_text,new_text)
    exec(compile(text,str(original),'exec'),{'__file__':str(original),'__name__':'__main__'})
print('PASS tag133 generated parent through existing one-shot failure suite; NO DEVICE')
