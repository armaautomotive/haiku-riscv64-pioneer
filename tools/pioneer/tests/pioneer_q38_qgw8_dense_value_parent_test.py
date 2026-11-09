#!/usr/bin/env python3
"""Run one-shot fault suite against the generated tag134 parent; no device."""
from pathlib import Path
import subprocess
import sys
import tempfile
here=Path(__file__).resolve().parent;original=here/'pioneer_q38_qgw8_trace_parent_test.py'
with tempfile.TemporaryDirectory(prefix='qgw8-value-parent-wrapper-',dir='/private/tmp') as directory:
    root=Path(directory);stage=root/'stage.sh';prior=root/'input.sh';parent=root/'value.sh'
    for script,source,target in (('build_pioneer_q38_qgw8_dense_stage_once.py',None,stage),
                                 ('build_pioneer_q38_qgw8_dense_input_once.py',stage,prior),
                                 ('build_pioneer_q38_qgw8_dense_value_once.py',prior,parent)):
        args=[sys.executable,str(here.parent/script)]
        if source is not None:args.append(str(source))
        subprocess.run(args+[str(target)],check=True)
    subprocess.run(['sh','-n',str(parent)],check=True)
    text=original.read_text();source_line="source=(here.parent/'pioneer_q38_qgw8_trace_physical_once.sh').read_text()"
    assert text.count(source_line)==1
    text=text.replace(source_line,'source=Path('+repr(str(parent))+').read_text()')
    changes=(
     ('e289d93fbecc569c249dacd1d72ed5d24eb73017ecfbc49c70c5f3e0a8a8e5e2','22c950146337def4ae733763cf50a2e8411ea3ba6313bfd090913fda3cb3017f'),
     ('34c6d9c21c3113d06979bc5e7feaffe6add4760a7b147d7fd6fd44f98f0207c3','eaa35bfbeed892e30128523b195e8ec29a9a1a0f6db724cbafb83f5e28117a4a'),
     ('bd48c957eab697f3b1a8bba84bf746a04f67694480418245d90dc66ab23e5f9c','f4747b8767b9ec6a0bbf7bb6b114f4b73d1b4d6f028040647f487af7a359128f'),
     ('pioneer-qgw8-trace-trial-20261109-a','pioneer-qgw8-dense-value-trial-20261109-a'),
     ('tenstorrent_pci_probe-qgw8-trace-131','tenstorrent_pci_probe-qgw8-dense-value-134'),
     ('pioneer-q38-qgw8-trace-candidate131','pioneer-q38-qgw8-dense-value-candidate134'),
     ('pioneer_q38_qgw8_trace_output.awk','pioneer_q38_qgw8_dense_value_output.awk'),
     ('2026110700010131','2026110700010134'),
     ("adminstate.write_text('2026102100010012 6\\n')","adminstate.write_text('2026102100010012 12\\n')"),
     ('echo "$2 7"','echo "$2 13"'),('echo "$2 8"','echo "$2 14"'),
     ('QGW8_TRACE_SNAPSHOT','QGW8_DENSE_VALUE_SNAPSHOT'),('generation=7; DATA_ONLY_AFTER_CLOSE','generation=13; DATA_ONLY_AFTER_CLOSE'),
     ('QGW8_TRACE_CHECK_ONLY_PASSED','QGW8_DENSE_VALUE_CHECK_ONLY_PASSED'),
    )
    for old,new in changes:
        assert old in text,old;text=text.replace(old,new)
    exec(compile(text,str(original),'exec'),{'__file__':str(original),'__name__':'__main__'})
print('PASS tag134 generated parent one-shot failure suite; NO DEVICE')
