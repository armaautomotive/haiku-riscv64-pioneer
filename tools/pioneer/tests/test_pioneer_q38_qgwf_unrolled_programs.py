#!/usr/bin/env python3
"""Authenticate the distinct QGWF unrolled native bundle; no device access."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert len(sys.argv)==6 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
root_a,root_b,worker_a,worker_b,old_worker=map(Path,sys.argv[1:]);here=Path(__file__).resolve().parent.parent
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (root_a,root_b,worker_a,worker_b,old_worker))
generator=here/'generate_qwen38_hybrid_programs.py';selector='--qgwf-unrolled-frontier-native-candidate'
with tempfile.TemporaryDirectory(prefix='qgwf-unroll-bundle-',dir='/private/tmp') as tmp:
    tmp=Path(tmp);outputs=[tmp/'a.h',tmp/'b.h']
    for root,worker,out in ((root_a,worker_a,outputs[0]),(root_b,worker_b,outputs[1])):
        subprocess.run([sys.executable,str(generator),selector,str(root),str(worker),str(out)],check=True)
    assert outputs[0].read_bytes()==outputs[1].read_bytes()
    assert hashlib.sha256(outputs[0].read_bytes()).hexdigest()=='c7182ca788b792067bc55ad373108bd76eeba2b00146af526f9e883576a5ebe9'
    inspection=[tmp/'inspect-a.h',tmp/'inspect-b.h'];inspect='--qgwf-unrolled-frontier-native-inspection'
    for root,worker,out in ((root_a,worker_a,inspection[0]),(root_b,worker_b,inspection[1])):
        subprocess.run([sys.executable,str(generator),inspect,str(root),str(worker),str(out)],check=True)
    assert inspection[0].read_bytes()==inspection[1].read_bytes()
    assert hashlib.sha256(inspection[0].read_bytes()).hexdigest()=='6455f12643172c1fa70df9271025f1b634011f5ea9c12612ec2e7fcbf3f9edd8'
    for i,(selectors,root,worker) in enumerate((([selector],root_a,old_worker),
                                                (['--qgwf-frontier-native-candidate'],root_a,worker_a),
                                                ([inspect],root_a,old_worker),([selector,inspect],root_a,worker_a),
                                                ([selector,selector],root_a,worker_a),
                                                ([selector,'--qgwf-frontier-native-candidate'],root_a,worker_a))):
        out=tmp/('bad'+str(i)+'.h')
        result=subprocess.run([sys.executable,str(generator),*selectors,str(root),str(worker),str(out)],capture_output=True)
        assert result.returncode and not out.exists()
print('PASS independent unrolled ROOT/worker bundle and mixed-identity refusals; NO DEVICE')
