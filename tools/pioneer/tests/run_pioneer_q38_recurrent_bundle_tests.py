#!/usr/bin/env python3
"""Rebuild the pinned recurrent program bundle; never open a device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==4 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root,worker,old_root=map(Path,sys.argv[1:]);assert all(p.is_absolute() and p.is_dir() for p in (root,worker,old_root))
generator=Path(__file__).resolve().parent.parent/'generate_qwen38_hybrid_programs.py'
with tempfile.TemporaryDirectory(prefix='q38-recurrent-bundle-',dir='/private/tmp') as tmp:
    folder=Path(tmp);outputs=[folder/'a.h',folder/'b.h']
    for out in outputs:
        subprocess.run([sys.executable,str(generator),'--qgwf-recurrent-native-inspection',str(root),str(worker),str(out)],check=True)
    assert outputs[0].read_bytes()==outputs[1].read_bytes()
    assert hashlib.sha256(outputs[0].read_bytes()).hexdigest()=='7b0da287da2afdfa1e9b74ad2faeda8a29363972143d95b8311cde2e80f878b9'
    for name,options,bad_root in (
        ('old.h',['--qgwf-recurrent-native-inspection'],old_root),
        ('ambiguous.h',['--qgwf-recurrent-native-inspection','--qgwf-frontier-native-inspection'],root),
    ):
        out=folder/name
        result=subprocess.run([sys.executable,str(generator),*options,str(bad_root),str(worker),str(out)],capture_output=True)
        assert result.returncode and not out.exists(),name
print('PASS recurrent ROOT/worker bundle reproducibility and mismatched-image refusal; NO DEVICE')
