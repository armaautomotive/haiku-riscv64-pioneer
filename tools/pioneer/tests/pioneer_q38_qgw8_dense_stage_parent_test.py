#!/usr/bin/env python3
"""Apply the existing one-shot fault suite to the actual generated tag132 parent."""
from pathlib import Path
import subprocess
import sys
import tempfile

here=Path(__file__).resolve().parent
original=here/'pioneer_q38_qgw8_trace_parent_test.py'
generator=here.parent/'build_pioneer_q38_qgw8_dense_stage_once.py'
with tempfile.TemporaryDirectory(prefix='qgw8-dense-parent-wrapper-',dir='/private/tmp') as directory:
    parent=Path(directory)/'actual-parent.sh'
    subprocess.run([sys.executable,str(generator),str(parent)],check=True)
    subprocess.run(['sh','-n',str(parent)],check=True)
    text=original.read_text()
    source_line="source=(here.parent/'pioneer_q38_qgw8_trace_physical_once.sh').read_text()"
    assert text.count(source_line)==1
    text=text.replace(source_line,'source=Path('+repr(str(parent))+').read_text()')
    changes=(
     ('e289d93fbecc569c249dacd1d72ed5d24eb73017ecfbc49c70c5f3e0a8a8e5e2','df4d8c24d3f3fd181e9433ae15af9141e7e73e3a78137e414ce1c704a3eacb53'),
     ('34c6d9c21c3113d06979bc5e7feaffe6add4760a7b147d7fd6fd44f98f0207c3','5988d972f927990efd1bd4e89f3125b922fcf82bf216988c1499346d0fe1f15a'),
     ('bd48c957eab697f3b1a8bba84bf746a04f67694480418245d90dc66ab23e5f9c','00b042d648c82ae84838a8739f72e0657426813066cbd010393a79c8c493ce2d'),
     ('pioneer-qgw8-trace-trial-20261109-a','pioneer-qgw8-dense-stage-trial-20261109-a'),
     ('tenstorrent_pci_probe-qgw8-trace-131','tenstorrent_pci_probe-qgw8-dense-stage-132'),
     ('pioneer-q38-qgw8-trace-candidate131','pioneer-q38-qgw8-dense-stage-candidate132'),
     ('pioneer_q38_qgw8_trace_output.awk','pioneer_q38_qgw8_dense_stage_output.awk'),
     ('2026110700010131','2026110700010132'),
     ("adminstate.write_text('2026102100010012 6\\n')","adminstate.write_text('2026102100010012 8\\n')"),
     ('echo "$2 7"','echo "$2 9"'),('echo "$2 8"','echo "$2 10"'),
     ('QGW8_TRACE_SNAPSHOT','QGW8_DENSE_STAGE_SNAPSHOT'),('generation=7; DATA_ONLY_AFTER_CLOSE','generation=9; DATA_ONLY_AFTER_CLOSE'),
     ('QGW8_TRACE_CHECK_ONLY_PASSED','QGW8_DENSE_STAGE_CHECK_ONLY_PASSED'),
    )
    for old,new in changes:
        assert old in text,old
        text=text.replace(old,new)
    exec(compile(text,str(original),'exec'),{'__file__':str(original),'__name__':'__main__'})
print('PASS tag132 generated parent through existing one-shot failure suite; NO DEVICE')
